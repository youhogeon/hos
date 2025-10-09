#ifndef __PRINT_H__
#define __PRINT_H__

#include "../types.h"

#define VGA_MEM ((volatile CHARACTER*)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_CELLS (VGA_COLS * VGA_ROWS)

#define CRTC_INDEX 0x3D4
#define CRTC_DATA 0x3D5
#define CRTC_CUR_LO 0x0F
#define CRTC_CUR_HI 0x0E
#define CRTC_START_LO 0x0D
#define CRTC_START_HI 0x0C

#define VGA_HW_BUFFER_CELLS 16384
#define CELL_MASK (VGA_HW_BUFFER_CELLS - 1)
#define WRAP(x) ((WORD)((x) & CELL_MASK))

#define DEFAULT_ATTR 0x07

int kPrint(const char* str);
void kPrintln(const char* str);
void kPrintErr(const char* str);

#pragma pack(push, 1)

typedef struct kCharactorStruct {
    BYTE bCharactor;
    BYTE bAttribute;
} CHARACTER;

#pragma pack(pop)

#endif /*__PRINT_H__*/