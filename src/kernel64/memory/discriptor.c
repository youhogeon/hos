#include "discriptor.h"
#include "../io/print.h"
#include "../util/memory.h"

////////////////////////////////////////////////////////////////
// TSS, GDT
////////////////////////////////////////////////////////////////

void kInitGDTAndTSS(void) {
    TSS* pstTSS = (TSS*)TSS_STARTADDRESS;
    kinitTSS(pstTSS);

    GDTR* pstGDTR = (GDTR*)GDTR_STARTADDRESS;
    GDTENTRY8* pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
    pstGDTR->wLimit = GDT_TOTAL_SIZE - 1;
    pstGDTR->qwBaseAddress = (QWORD)pstEntry;

    // GDT 테이블 생성
    // NULL, TSS, Code, Data
    kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);
    kSetGDTEntry16((GDTENTRY16*)&(pstEntry[1]),
                   (QWORD)pstTSS,
                   sizeof(TSS) - 1,
                   GDT_FLAGS_UPPER_TSS,
                   GDT_FLAGS_LOWER_TSS,
                   GDT_TYPE_TSS);
    kSetGDTEntry8(&(pstEntry[3]), 0, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
    kSetGDTEntry8(&(pstEntry[4]), 0, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
}

void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags,
                   BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
    pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
    pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}

void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags,
                    BYTE bType) {
    pstEntry->wLowerLimit = dwLimit & 0xFFFF;
    pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
    pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
    pstEntry->bTypeAndLowerFlag = bLowerFlags | bType;
    pstEntry->bUpperLimitAndUpperFlag = ((dwLimit >> 16) & 0xFF) | bUpperFlags;
    pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
    pstEntry->dwUpperBaseAddress = qwBaseAddress >> 32;
    pstEntry->dwReserved = 0;
}

void kinitTSS(TSS* pstTSS) {
    kMemSet(pstTSS, 0, sizeof(TSS));
    pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE;
    pstTSS->wIOMapBaseAddress = 0xFFFF; // disable I/O Map
}

////////////////////////////////////////////////////////////////
// IDT
////////////////////////////////////////////////////////////////

void kInitIDT(void) {
    IDTR* pstIDTR;
    IDTENTRY* pstEntry;
    int i;

    // IDTR의 시작 어드레스
    pstIDTR = (IDTR*)IDTR_STARTADDRESS;
    // IDT 테이블의 정보 생성
    pstEntry = (IDTENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
    pstIDTR->qwBaseAddress = (QWORD)pstEntry;
    pstIDTR->wLimit = IDT_TOTAL_SIZE - 1;

    // 0~99까지 벡터를 모두 DummyHandler로 연결
    for (i = 0; i < IDT_MAXENTRYCOUNT; i++) {
        kSetIDTEntry(&(pstEntry[i]),
                     kDummyHandler,
                     GDT_KERNELCODESEGMENT,
                     IDT_FLAGS_IST1,
                     IDT_FLAGS_KERNEL,
                     IDT_TYPE_INTERRUPT);
    }
}

void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType) {
    pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xFFFF;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x3;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xFFFF;
    pstEntry->dwUpperBaseAddress = (QWORD)pvHandler >> 32;
    pstEntry->dwReserved = 0;
}

// tmp
void kDummyHandler(void) {
    kPrintln("====================================================");
    kPrintln("          Dummy Interrupt Handler Execute~!!!       ");
    kPrintln("           Interrupt or Exception Occur~!!!!        ");
    kPrintln("====================================================");

    while (1)
        ;
}