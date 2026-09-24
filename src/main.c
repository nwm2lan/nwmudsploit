#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "../kernelhaxcode_3ds/takeover.h"
#include "kernelhaxcode_3ds_bin.h"

Result udsploit(void);
void print(char *msg, ...);

PrintConsole topScreenConsole;

static void drawGlyph(u16 codepoint, int x, int y, u32 color)
{
    // This fallback keeps the program compiling even if a font is unavailable.
    // The real font should be provided by a project header or a generated bitmap font.
    if (codepoint < 0x80) {
        gfxDrawPixel(x, y, color);
        return;
    }

    // Draw a simple placeholder for non-ASCII glyphs so the program still runs.
    for (int py = 0; py < 8; ++py) {
        for (int px = 0; px < 8; ++px) {
            if ((py + px) & 1) {
                gfxDrawPixel(x + px, y + py, color);
            }
        }
    }
}

static unsigned utf8ToCodepoint(const char **ptr)
{
    const unsigned char *p = (const unsigned char *)(*ptr);
    unsigned codepoint = 0;

    if ((p[0] & 0x80u) == 0u) {
        codepoint = p[0];
        *ptr = (const char *)(p + 1);
        return codepoint;
    }

    if ((p[0] & 0xE0u) == 0xC0u) {
        codepoint = ((unsigned)(p[0] & 0x1Fu) << 6) |
                    (unsigned)(p[1] & 0x3Fu);
        *ptr = (const char *)(p + 2);
        return codepoint;
    }

    if ((p[0] & 0xF0u) == 0xE0u) {
        codepoint = ((unsigned)(p[0] & 0x0Fu) << 12) |
                    ((unsigned)(p[1] & 0x3Fu) << 6) |
                    ((unsigned)(p[2] & 0x3Fu));
        *ptr = (const char *)(p + 3);
        return codepoint;
    }

    *ptr = (const char *)(p + 1);
    return '?';
}

static void drawUTF8String(const char *msg, int x, int y, u32 color)
{
    int cursorX = x;
    int cursorY = y;

    while (*msg != '\0') {
        const char *next = msg;
        unsigned codepoint = utf8ToCodepoint(&next);

        if (codepoint == '\n') {
            cursorX = x;
            cursorY += 12;
            msg = next;
            continue;
        }

        if (codepoint == '\r') {
            msg = next;
            continue;
        }

        if (codepoint == '\t') {
            cursorX += 24;
            msg = next;
            continue;
        }

        drawGlyph((u16)codepoint, cursorX, cursorY, color);
        cursorX += 6;
        msg = next;
    }
}

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
        print("キャンセルしました。");
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

static int y = 0;

void print(char *msg, ...)
{
    va_list args;
    char s[256] = {0};

    va_start(args, msg);
    vsnprintf(s, sizeof(s), msg, args);
    va_end(args);

    drawUTF8String(s, 0, y * 12, 0xFFFF);
    gfxSwapBuffers();
    ++y;
}
