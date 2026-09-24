#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <3ds.h>

// https://rgbcolorpicker.com/0-1
#define ERROR_COLOR 0b1001
#define SUCCESS_COLOR 0b1010
#define WHITE_COLOR 0b1111



Result udsploit();
Result hook_kernel();

void print(char *msg, ...);

Result res = 0;
PrintConsole topScreenConsole;

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, &topScreenConsole);
    topScreenConsole.bg = ERROR_COLOR;
    topScreenConsole.fg = WHITE_COLOR;

    consoleClear();

	res = udsploit();
	if(R_SUCCESS(res){
		res = hook_kernel();
		print("Success");
	}else{
		print("SHIPPAI");
	}



    print("Exit: START Button");

    while (aptMainLoop())
    {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
			break;
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
    vsprintf(s, msg, args);
    printf("\x1b[%u;1H %s", ++y, s);
    va_end(args);
}
