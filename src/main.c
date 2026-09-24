#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "../kernelhaxcode_3ds/takeover.h"
#include "kernelhaxcode_3ds_bin.h"

Result udsploit(void);
void print(char *msg, ...);

PrintConsole topScreenConsole;

static Result takeOverKernelAndBeyond(const char *payloadFileName, size_t payloadFileOffset)
{
    __dsb();
    BlobLayout *layout = (BlobLayout *)linearMemAlign(sizeof(BlobLayout), 0x1000);
    if (layout == NULL)
        return -1;

    memset(layout, 0, sizeof(BlobLayout));
    memcpy(layout->code, kernelhaxcode_3ds_bin, kernelhaxcode_3ds_bin_size);
    khc3dsPrepareL2Table(layout);
    // Ensure everything (esp. the layout) is written back into the main memory
    GSPGPU_FlushDataCache((const void *)0x14000000, 0x700000);
    __dsb();
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
        print("カーネルを乗っ取っています: 0x%08lX", ret);
    } else {
        print("失敗");
    }

    if (R_SUCCEEDED(ret)) {
        print("完了。");
    } else if (ret == RES_USER_CANCELED) {
        printf("キャンセルしました。\n");
    }

    print("終了: START ボタン");

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
