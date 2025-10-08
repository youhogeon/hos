#include "memory.h"
#include "types.h"

BOOL kInitMemory(void) {
    // Check Memort size (at least 64MB)
    for (DWORD* addr = (DWORD*)MEMORY_PAGE_BEGIN; addr < (DWORD*)MEMORY_REQUIREMENT; addr += 0x100000) {
        *addr = 0x12345678;
        if (*addr != 0x12345678) {
            return FALSE;
        }
    }

    // Clear Memory (1MB - 6MB)
    DWORD* currentAddr = (DWORD*)MEMORY_PAGE_BEGIN;
    DWORD* maxAddr = (DWORD*)MEMORY_K64_END;

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
    DWORD* sourceAddr = (DWORD*)MEMORY_K64_SOURCE;
    DWORD* endAddr = (DWORD*)MEMORY_PAGE_BEGIN;
    DWORD* destAddr = (DWORD*)MEMORY_K64_BEGIN;

    while (sourceAddr < endAddr) {
        *destAddr = *sourceAddr;
        sourceAddr++;
        destAddr++;
    }
}