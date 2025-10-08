#ifndef __PRINT_H__
#define __PRINT_H__

#include "types.h"

#define VGA_MEM ((CHARACTER*)0xB8000)
#define VGA_COLS 80
#define CRTC_INDEX 0x3D4
#define CRTC_DATA 0x3D5
#define CRTC_CUR_LO 0x0F
#define CRTC_CUR_HI 0x0E

void kPrintln(const char* str);
void kPrintErr(const char* str);

#pragma pack(push, 1)

typedef struct kCharactorStruct {
    BYTE bCharactor;
    BYTE bAttribute;
} CHARACTER;

#pragma pack(pop)

#endif /*__PRINT_H__*/