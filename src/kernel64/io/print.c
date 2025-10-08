#include "print.h"
#include "../util/assembly.h"

static WORD _readCursorPos(void) {
    outb(CRTC_INDEX, CRTC_CUR_LO);
    WORD lo = inb(CRTC_DATA);
    outb(CRTC_INDEX, CRTC_CUR_HI);
    WORD hi = inb(CRTC_DATA);

    return (WORD)((hi << 8) | lo);
}

static void _setCursorPos(WORD pos) {
    outb(CRTC_INDEX, CRTC_CUR_LO);
    outb(CRTC_DATA, (BYTE)(pos & 0xFF));
    outb(CRTC_INDEX, CRTC_CUR_HI);
    outb(CRTC_DATA, (BYTE)((pos >> 8) & 0xFF));
}

int kPrint(const char* str) {
    CHARACTER* pstScreen = VGA_MEM;

    int pos = _readCursorPos();
    pstScreen += pos;

    int i = 0;
    for (i = 0; str[i] != 0; i++) {
        pstScreen[i].bCharactor = str[i];
    }

    _setCursorPos(pos + i);

    return i;
}

void kPrintln(const char* str) {
    int len = kPrint(str);
    WORD pos = _readCursorPos();
    pos += (VGA_COLS - (pos % VGA_COLS));

    _setCursorPos(pos);
}

void kPrintErr(const char* str) {
    CHARACTER* pstScreen = VGA_MEM;
    int pos = _readCursorPos();
    pstScreen += pos;

    kPrintln(str);

    for (int i = 0; str[i] != 0; i++) {
        pstScreen[i].bAttribute = 0x4F;
    }
}
