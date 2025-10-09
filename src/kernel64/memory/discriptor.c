#include "discriptor.h"
#include "../interrupt/ISR.h"
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

    // Exception ISR
    kSetIDTEntry(&(pstEntry[0]),
                 kISR_DivideError,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[1]),
                 kISR_Debug,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[2]), kISR_NMI, GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[3]),
                 kISR_BreakPoint,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[4]),
                 kISR_Overflow,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[5]),
                 kISR_BoundRangeExceeded,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[6]),
                 kISR_InvalidOpcode,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[7]),
                 kISR_DeviceNotAvailable,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[8]),
                 kISR_DoubleFault,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[9]),
                 kISR_CoprocessorSegmentOverrun,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[10]),
                 kISR_InvalidTSS,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[11]),
                 kISR_SegmentNotPresent,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[12]),
                 kISR_StackSegmentFault,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[13]),
                 kISR_GeneralProtection,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[14]),
                 kISR_PageFault,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[15]), kISR_15, GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[16]),
                 kISR_FPUError,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[17]),
                 kISR_AlignmentCheck,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[18]),
                 kISR_MachineCheck,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[19]),
                 kISR_SIMDError,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[20]),
                 kISR_ETCException,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);

    for (i = 21; i < 32; i++) {
        kSetIDTEntry(&(pstEntry[i]),
                     kISR_ETCException,
                     GDT_KERNELCODESEGMENT,
                     IDT_FLAGS_IST1,
                     IDT_FLAGS_KERNEL,
                     IDT_TYPE_INTERRUPT);
    }

    // Interrupt ISR
    kSetIDTEntry(&(pstEntry[32]),
                 kISR_Timer,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[33]),
                 kISR_Keyboard,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[34]),
                 kISR_SlavePIC,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[35]),
                 kISR_Serial2,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[36]),
                 kISR_Serial1,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[37]),
                 kISR_Parallel2,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[38]),
                 kISR_Floppy,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[39]),
                 kISR_Parallel1,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[40]),
                 kISR_RTC,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[41]),
                 kISR_Reserved,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[42]),
                 kISR_NotUsed1,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[43]),
                 kISR_NotUsed2,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[44]),
                 kISR_Mouse,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[45]),
                 kISR_Coprocessor,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[46]),
                 kISR_HDD1,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);
    kSetIDTEntry(&(pstEntry[47]),
                 kISR_HDD2,
                 GDT_KERNELCODESEGMENT,
                 IDT_FLAGS_IST1,
                 IDT_FLAGS_KERNEL,
                 IDT_TYPE_INTERRUPT);

    for (i = 48; i < IDT_MAXENTRYCOUNT; i++) {
        kSetIDTEntry(&(pstEntry[i]),
                     kISR_ETCInterrupt,
                     GDT_KERNELCODESEGMENT,
                     IDT_FLAGS_IST1,
                     IDT_FLAGS_KERNEL,
                     IDT_TYPE_INTERRUPT);
    }
}

void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType) {
    pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xFFFF;
    pstEntry->wSegmentSelector = wSelector;
    pstEntry->bIST = bIST & 0x7;
    pstEntry->bTypeAndFlags = bType | bFlags;
    pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xFFFF;
    pstEntry->dwUpperBaseAddress = (QWORD)pvHandler >> 32;
    pstEntry->dwReserved = 0;
}