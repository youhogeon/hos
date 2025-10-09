#include "video.h"
#include "../util/assembly.h"
#include <stdarg.h>

static WORD gStartAddr = 0;
static int gScrollCount = 0;

static inline WORD _readCursorPos(void) {
    outb(CRTC_INDEX, CRTC_CUR_LO);
    WORD lo = inb(CRTC_DATA);
    outb(CRTC_INDEX, CRTC_CUR_HI);
    WORD hi = inb(CRTC_DATA);
    return WRAP((hi << 8) | lo);
}
static inline void _setCursorPos(WORD pos) {
    pos = WRAP(pos);
    outb(CRTC_INDEX, CRTC_CUR_HI);
    outb(CRTC_DATA, (BYTE)(pos >> 8));
    outb(CRTC_INDEX, CRTC_CUR_LO);
    outb(CRTC_DATA, (BYTE)(pos & 0xFF));
}
static inline void _setStartAddrHW(WORD addr) {
    outb(CRTC_INDEX, CRTC_START_HI);
    outb(CRTC_DATA, (BYTE)(addr >> 8));
    outb(CRTC_INDEX, CRTC_START_LO);
    outb(CRTC_DATA, (BYTE)(addr & 0xFF));
}
static inline void _setStartAddr(WORD addr) {
    gStartAddr = WRAP(addr);
    _setStartAddrHW(gStartAddr);
}

// Visual helpers (ring-based)
static inline WORD _ringDistance(WORD from, WORD to) { return (WORD)((to - from) & CELL_MASK); }
static inline WORD _visualCol(WORD pos) { return (WORD)(_ringDistance(gStartAddr, pos) % VGA_COLS); }
static inline WORD _lineStart(WORD pos) { return WRAP(pos - _visualCol(pos)); }
static inline WORD _advanceToNextLine(WORD pos) { return WRAP(pos + (VGA_COLS - _visualCol(pos))); }

// Drawing
static inline void _clearBottomLine(void) {
    volatile CHARACTER* vram = VGA_MEM;
    WORD base = WRAP(gStartAddr + (VGA_ROWS - 1) * VGA_COLS);
    for (int x = 0; x < VGA_COLS; x++) {
        WORD p = WRAP(base + x);
        vram[p].bCharactor = ' ';
        vram[p].bAttribute = VGA_ATTR_DEFAULT;
    }
}

// NORMALIZE
static void _normalizeViewport(WORD* pCursorAbs) {
    WORD visOff = _ringDistance(gStartAddr, *pCursorAbs);
    volatile CHARACTER* vram = VGA_MEM;

    WORD src0 = gStartAddr;
    WORD len0 = (WORD)((VGA_HW_BUFFER_CELLS - gStartAddr) < VGA_CELLS ? (VGA_HW_BUFFER_CELLS - gStartAddr) : VGA_CELLS);
    WORD len1 = (WORD)(VGA_CELLS - len0);

    for (WORD i = 0; i < len0; i++) {
        vram[i].bCharactor = vram[WRAP(src0 + i)].bCharactor;
        vram[i].bAttribute = vram[WRAP(src0 + i)].bAttribute;
    }
    for (WORD i = 0; i < len1; i++) {
        vram[len0 + i].bCharactor = vram[i].bCharactor;
        vram[len0 + i].bAttribute = vram[i].bAttribute;
    }

    _setStartAddr(0);
    *pCursorAbs = WRAP(visOff);
    _setCursorPos(*pCursorAbs);

    gScrollCount = 0;
}

// ===== Scroll =====
static inline void _maybeScroll(WORD* pCursorAbs) {
    if (_ringDistance(gStartAddr, *pCursorAbs) >= VGA_CELLS) {
        _setStartAddr(WRAP(gStartAddr + VGA_COLS));
        _setCursorPos(*pCursorAbs);
        _clearBottomLine();
        if (++gScrollCount >= 128) {
            _normalizeViewport(pCursorAbs);
        }
    }
}

static void _putChar(WORD* pCursorAbs, char ch, BYTE attr) {
    if (ch == '\n') {
        *pCursorAbs = _advanceToNextLine(*pCursorAbs);
        _maybeScroll(pCursorAbs);
        _setCursorPos(*pCursorAbs);
        return;
    } else if (ch == '\r') {
        *pCursorAbs = _lineStart(*pCursorAbs);
        _setCursorPos(*pCursorAbs);
        return;
    } else if (ch == '\t') {
        int spaces = 4 - (_visualCol(*pCursorAbs) & 3);
        for (int i = 0; i < spaces; i++)
            _putChar(pCursorAbs, ' ', attr);
        return;
    }

    volatile CHARACTER* vram = VGA_MEM;
    WORD p = WRAP(*pCursorAbs);
    vram[p].bCharactor = (BYTE)ch;
    vram[p].bAttribute = attr;

    *pCursorAbs = WRAP(*pCursorAbs + 1);
    _maybeScroll(pCursorAbs);
    _setCursorPos(*pCursorAbs);
}

// API
int kPrint(const char* str) { return kPrintColor(str, VGA_ATTR_DEFAULT); }

int kPrintColor(const char* str, BYTE attr) {
    WORD pos = _readCursorPos();
    int i = 0;
    for (; str[i] != 0; i++) {
        _putChar(&pos, str[i], attr);
    }
    return i;
}

int kPrintf(const char* pcFormatString, ...) {
    va_list ap;
    char pcBuffer[1024];
    int i;

    va_start(ap, pcFormatString);

    i = kVSPrintf(pcBuffer, pcFormatString, ap);
    va_end(ap);

    kPrint(pcBuffer);

    return i;
}

void kPrintErr(const char* str) { kPrintColor(str, VGA_ATTR_ERROR); }

void kPrintln(const char* str) { kPrintlnColor(str, VGA_ATTR_DEFAULT); }

void kPrintlnColor(const char* str, BYTE attr) {
    if (str[0] == 0) {
        kPrint("\n");
        return;
    }

    WORD pos = _readCursorPos();
    for (int i = 0; str[i] != 0; i++) {
        _putChar(&pos, str[i], attr);
    }

    // prevent duplicated newline
    WORD col = _visualCol(pos = _readCursorPos());
    if (col != 0)
        _putChar(&pos, '\n', attr);
}

void kPrintAt(int x, int y, const char* str) {
    if (x < 0 || x >= VGA_COLS || y < 0 || y >= VGA_ROWS)
        return;

    WORD pos = WRAP(gStartAddr + y * VGA_COLS + x);
    for (int i = 0; str[i] != 0; i++) {
        volatile CHARACTER* vram = VGA_MEM;
        WORD p = WRAP(pos + i);
        vram[p].bCharactor = str[i];
        vram[p].bAttribute = VGA_ATTR_DEFAULT;
    }
}

void kClear(int clearStart) {
    if (clearStart < 0)
        clearStart = 0;
    if (clearStart > VGA_ROWS)
        clearStart = VGA_ROWS;

    volatile CHARACTER* vram = VGA_MEM;

    for (int y = clearStart; y < VGA_ROWS; y++) {
        WORD base = WRAP(gStartAddr + y * VGA_COLS);
        for (int x = 0; x < VGA_COLS; x++) {
            WORD p = WRAP(base + x);
            vram[p].bCharactor = ' ';
            vram[p].bAttribute = VGA_ATTR_DEFAULT;
        }
    }

    WORD cursor = WRAP(gStartAddr + clearStart * VGA_COLS);
    _setCursorPos(cursor);

    gScrollCount = 0;
}

void kClearChar() {
    WORD pos = _readCursorPos();
    if (pos == 0)
        return;

    volatile CHARACTER* vram = VGA_MEM;
    pos = WRAP(pos - 1);
    vram[pos].bCharactor = ' ';
    vram[pos].bAttribute = VGA_ATTR_DEFAULT;
    _setCursorPos(pos);
}