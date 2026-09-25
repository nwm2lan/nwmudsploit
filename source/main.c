#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>
#include <3ds/types.h>

#include "../kernelhaxcode_3ds/takeover.h"
#include "kernelhaxcode_3ds_bin.h"

const char *yellow="\x1b[33;1m";
const char *blue="\x1b[34;1m";
const char *dblue="\x1b[34;0m";
const char *white="\x1b[37;1m";

Result udsploit(void);

inline void __flush_prefetch_buffer(void)
{
    // Similar to isb in newer Arm architecture versions
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory");
}

// Source: https://github.com/smealum/udsploit/blob/master/source/kernel.c#L11
void gspSetTextureCopyPhys(u32 outPa, u32 inPa, u32 size, u32 inDim, u32 outDim, u32 flags)
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

void mapL2TableViaGpuDma(const BlobLayout *layout, void *workBuffer)
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

Result takeOverKernelAndBeyond(const char *payloadFileName, size_t payloadFileOffset)
{
    BlobLayout *layout = (BlobLayout *)linearMemAlign(sizeof(BlobLayout), 0x1000);
    if (layout == NULL) {
        return -1;
    }

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

int iscfw=0;

void chk_cfw(){

	Result res;
    
    u8 *data; 
    char path[0x200]={0};
	u8 ctrpath[0x200]={0};
	memset(ctrpath, 0, 0x200); //make damned sure this is safe
	memset(path, 0, 0x200); 
	
	data=(u8*)malloc(0x10000);

	res = CFG_GetConfigInfoBlk4(2, 0, data);
	if(res){
		printf("Error: cfg:s or cfg:i not available, abort\n");
		while(1) svcSleepThread(17*1000*1000);
	}
	res = FSUSER_ExportIntegrityVerificationSeed((FS_IntegrityVerificationSeed*)(data+0x8000)); //data outputed is don't care, we're just testing for cfw. hax* userland would never be allowed to call this.
	if(!res){
		iscfw=1;
		res = FSUSER_GetSdmcCtrRootPath(ctrpath, 0x80*2);
		for(int i=0;i<0x180;i++){
			path[i]=ctrpath[i*2];
			if(!path[i]) break;
		}
	}
    
    free(data);
    
}

Result exploit(){
    
    Result ret;
    
    ret = udsploit();
    if (R_SUCCEEDED(ret)) {
        printf("Taking over kernel: 0x%08lX", ret);
        return 0;
    }else{
        return -1;
    }
}

int cursor=0;
int menu(u32 n){

	Result res;
    
    consoleClear();
	
	printf("udsploit Loader \nSTATUS: %s\n\n", iscfw ? "cfw":"user");

	char *choices[]={
		"ZIKKOU      udsploit",
		"ZIKKOU      boot.bin",
		"SYUURYOU    to menu",
	};
	
	int maxchoices=sizeof(choices)/4;
	
	if(n & KEY_UP) cursor--;
	else if (n & KEY_DOWN) cursor++;
	if (cursor >= maxchoices) cursor=0;
	else if (cursor < 0) cursor=maxchoices-1;
	
	
	for(int i=0; i<maxchoices; i++){
		printf("%s%s%s\n", cursor==i ? yellow:white, choices[i], white);
	}
	
	printf("--------------------------------------------------");
	printf(" \n");
	
	if(n & KEY_A) {
		
		switch(cursor){
			case 0:
                res = exploit();
                if(res == -1){
                    break;
                }
			break;
            case 1:
                res = takeOverKernelAndBeyond("boot.bin", 0);
                if (R_SUCCEEDED(res)) {
                    return 0;
                }else{
                    return -1;
                }
			case 2:
			    return 1;
			break;
			
			default:;
		};
		svcSleepThread(500*1000*1000);
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	Result res;
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	cfguInit();
	nsInit();
	fsInit();

	u32 kDown;
	
	hidScanInput();
	kDown = hidKeysDown();
	if(kDown & KEY_B){
	    chk_cfw();
	}else{
		printf("check: skip");
		svcSleepThread(500*1000*1000);
	}
    
	//printf("%08X\n",(int)res);
	//printf("%s\n",(char*)path);
	//printf("\n");
	
	menu(0);

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		kDown = hidKeysDown();
		if(kDown & 0xfff){
			//if(fail) break;
			res = menu(kDown);
			if(res) break;
		}
	}

	gfxExit();
	return 0;
}
