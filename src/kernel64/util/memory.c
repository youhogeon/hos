#include "memory.h"

void kMemSet(void* pvDestination, BYTE bData, int iSize) {
    int i;

    for (i = 0; i < iSize; i++) {
        ((char*)pvDestination)[i] = bData;
    }
}

int kMemCpy(void* pvDestination, const void* pvSource, int iSize) {
    int i;

    for (i = 0; i < iSize; i++) {
        ((char*)pvDestination)[i] = ((char*)pvSource)[i];
    }

    return iSize;
}

int kMemCmp(const void* pvDestination, const void* pvSource, int iSize) {
    int i;
    char cTemp;

    for (i = 0; i < iSize; i++) {
        cTemp = ((char*)pvDestination)[i] - ((char*)pvSource)[i];
        if (cTemp != 0) {
            return (int)cTemp;
        }
    }

    return 0;
}

static int gs_qwTotalRAMMBSize = 0;

QWORD kMemSize() {
    if (gs_qwTotalRAMMBSize != 0) {
        return gs_qwTotalRAMMBSize;
    }

    // 64Mbyte(0x4000000)부터 4Mbyte단위로 검사 시작
    DWORD* pdwCurrentAddress = (DWORD*)0x4000000;
    while (1) {
        DWORD dwPreviousValue = *pdwCurrentAddress;

        *pdwCurrentAddress = 0x12345678;
        if (*pdwCurrentAddress != 0x12345678) {
            break;
        }

        *pdwCurrentAddress = dwPreviousValue;

        pdwCurrentAddress += 0x100000;
    }

    return (QWORD)pdwCurrentAddress;
}