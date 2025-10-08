#include "types.h"

BOOL kInitMemory(void) {
    // Check Memort size (at least 64MB)
    for (DWORD* addr = (DWORD*)0x100000; addr < (DWORD*)0x4000000; addr += 0x100000) {
        *addr = 0x12345678;
        if (*addr != 0x12345678) {
            return FALSE;
        }
    }

    // Clear Memory (1MB - 6MB)
    DWORD* currentAddr = (DWORD*)0x100000;
    DWORD* maxAddr = (DWORD*)0x600000;

    while (currentAddr < maxAddr) {
        *currentAddr = 0;
        if (*currentAddr != 0) {
            return FALSE;
        }
        currentAddr++;
    }

    return TRUE;
}

void copyKernel64ImageTo2MB(void) {
    DWORD* sourceAddr = (DWORD*)0x11000;
    DWORD* endAddr = (DWORD*)0x100000;
    DWORD* destAddr = (DWORD*)0x200000;

    while (sourceAddr < endAddr) {
        *destAddr = *sourceAddr;
        sourceAddr++;
        destAddr++;
    }
}