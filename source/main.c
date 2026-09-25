#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #include <3ds.h>
#include <3ds/srv.h>
#include <3ds/font.h>
#include <3ds/console.h>
#include <3ds/gfx.h>
#include <3ds/svc.h>
#include <3ds/types.h>

#include "kernel_patch.h"

#include "../kernelhaxcode_3ds/takeover.h"
#include "kernelhaxcode_3ds_bin.h"

#ifndef DEFAULT_PAYLOAD_FILE_OFFSET
#define DEFAULT_PAYLOAD_FILE_OFFSET 0
#endif
#ifndef DEFAULT_PAYLOAD_FILE_NAME
#define DEFAULT_PAYLOAD_FILE_NAME   "SafeB9SInstaller.bin"
#endif

#define PRINT_WRITE  0, 0, 0
#define PRINT_GREEN  0, 255, 0
#define PRINT_RED    255, 0, 0

// https://rgbcolorpicker.com/0-1
#define BLACK_COLOR 0b0000
#define RED_COLOR   0b0101

const char *yellow="\x1b[33;1m";
const char *white="\x1b[37;1m";

Result udsploit(void);

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
    
	gspSetLcdFill(gspHandle, true, PRINT_WHITE);
	
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
				gspSetLcdFill(gspHandle, true, PRINT_GRREEN);
                res = doPayload(DEFAULT_PAYLOAD_FILE_NAME, DEFAULT_PAYLOAD_FILE_OFFSET);
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

PrintConsole topScreenConsole;

int main()
{
	Result res;
	u32 kDown;
	
	gfxInitDefault();
	consoleInit(GFX_TOP, &topScreenConsole);
	topScreenConsole.bg = RED_COLOR;
	topScreenConsole.fg = BLACK_COLOR;
	
	consoleClear();

	// chk
	cfguInit();
	fsInit();

	hidScanInput();
	kDown = hidKeysDown();
	if(kDown & KEY_B){
		printf("check: skip");
		svcSleepThread(500*1000*1000);
	}else{
	    chk_cfw();
	}
	
	menu(0);

	// Main loop
	while (aptMainLoop())
	{
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
		
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
