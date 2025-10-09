#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "../types.h"

////////////////////////////////////////////////////////////////
// TSS, GDT
////////////////////////////////////////////////////////////////

// Flags
#define GDT_TYPE_CODE 0x0A
#define GDT_TYPE_DATA 0x02
#define GDT_TYPE_TSS 0x09
#define GDT_FLAGS_LOWER_S 0x10
#define GDT_FLAGS_LOWER_DPL0 0x00
#define GDT_FLAGS_LOWER_DPL1 0x20
#define GDT_FLAGS_LOWER_DPL2 0x40
#define GDT_FLAGS_LOWER_DPL3 0x60
#define GDT_FLAGS_LOWER_P 0x80
#define GDT_FLAGS_UPPER_L 0x20
#define GDT_FLAGS_UPPER_DB 0x40
#define GDT_FLAGS_UPPER_G 0x80

#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)

#define GDT_FLAGS_UPPER_CODE (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA (GDT_FLAGS_UPPER_G)
#define GDT_FLAGS_UPPER_TSS (GDT_FLAGS_UPPER_G)

// Constants
#define GDT_TOTAL_SIZE (sizeof(GDTENTRY8) * 3 + sizeof(GDTENTRY16) * 1) // NULL, TSS, Code, Data

#define GDT_NULLSEGMENT 0x00
#define GDT_TSSSEGMENT 0x08
#define GDT_KERNELCODESEGMENT 0x18
#define GDT_KERNELDATASEGMENT 0x20

#define TSS_STARTADDRESS 0x142000
#define TSS_SIZE (sizeof(TSS))

#define GDTR_STARTADDRESS (TSS_STARTADDRESS + TSS_SIZE)

////////////////////////////////////////////////////////////////
// IDT, IST
////////////////////////////////////////////////////////////////

// Flags
#define IDT_TYPE_INTERRUPT 0x0E
#define IDT_TYPE_TRAP 0x0F
#define IDT_FLAGS_DPL0 0x00
#define IDT_FLAGS_DPL1 0x20
#define IDT_FLAGS_DPL2 0x40
#define IDT_FLAGS_DPL3 0x60
#define IDT_FLAGS_P 0x80
#define IDT_FLAGS_IST0 0
#define IDT_FLAGS_IST1 1

#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// Constants
#define IDT_MAXENTRYCOUNT 100
#define IDTR_STARTADDRESS 0x143000
#define IDT_STARTADDRESS (IDTR_STARTADDRESS + sizeof(IDTR))
#define IDT_TOTAL_SIZE (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

#define IST_STARTADDRESS 0x700000
#define IST_SIZE 0x100000

////////////////////////////////////////////////////////////////
// Structs
////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

typedef struct kGDTRStruct {
    WORD wLimit;
    QWORD qwBaseAddress;

    // padding for alignment
    WORD wPading;
    DWORD dwPading;
} GDTR, IDTR;

typedef struct kGDTEntry8Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bUpperBaseAddress1;

    BYTE bTypeAndLowerFlag;       // 4비트 Type, 1비트 S, 2비트 DPL, 1비트 P
    BYTE bUpperLimitAndUpperFlag; // 4비트 Segment Limit, 1비트 AVL, L, D/B, G

    BYTE bUpperBaseAddress2;
} GDTENTRY8;

typedef struct kGDTEntry16Struct {
    WORD wLowerLimit;
    WORD wLowerBaseAddress;
    BYTE bMiddleBaseAddress1;

    BYTE bTypeAndLowerFlag;       // 4비트 Type, 1비트 0, 2비트 DPL, 1비트 P
    BYTE bUpperLimitAndUpperFlag; // 4비트 Segment Limit, 1비트 AVL, 0, 0, G

    BYTE bMiddleBaseAddress2;
    DWORD dwUpperBaseAddress;

    DWORD dwReserved;
} GDTENTRY16;

typedef struct kTSSDataStruct {
    DWORD dwReserved1;
    QWORD qwRsp[3];
    QWORD qwReserved2;
    QWORD qwIST[7];
    QWORD qwReserved3;
    WORD wReserved;
    WORD wIOMapBaseAddress;
} TSS;

typedef struct kIDTEntryStruct {
    WORD wLowerBaseAddress;
    WORD wSegmentSelector;

    BYTE bIST;          // 3비트 IST, 5비트 0
    BYTE bTypeAndFlags; // 4비트 Type, 1비트 0, 2비트 DPL, 1비트 P

    WORD wMiddleBaseAddress;
    DWORD dwUpperBaseAddress;
    DWORD dwReserved;
} IDTENTRY;

#pragma pack(pop)

////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////

void kInitGDTAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags,
                   BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags,
                    BYTE bType);
void kinitTSS(TSS* pstTSS);

void kInitIDT(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);

#endif