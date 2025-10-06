#include "print.h"

static inline void outb(WORD port, BYTE val) {
    __asm__ volatile ("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline BYTE inb(WORD port) {
    BYTE ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));

    return ret;
}

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

void kPrintln(const char* str) {
    CHARACTER* pstScreen = VGA_MEM;
    
    int pos = _readCursorPos();
    pstScreen += pos;
    
    for (int i = 0; str[i] != 0; i++) {
        pstScreen[i].bCharactor = str[i];
    }

    _setCursorPos(pos + VGA_COLS);
}
