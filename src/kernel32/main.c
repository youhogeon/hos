#include "print.h"
#include "types.h"

BOOL initMemory(void);

void _start( void ) {
    kPrintln("Kenrel32 Initializing...");

    if (initMemory() == FALSE) {
        kPrintln("Memory Initialization Failed.");
        goto end;
    }

    kPrintln("Kernel32 Initialized.");

    end:
    while(1) ;
}

BOOL initMemory(void) {
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