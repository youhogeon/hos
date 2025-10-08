#include "memory.h"
#include "print.h"
#include "types.h"
#include "paging.h"
#include "cpu.h"
#include "k64switch.h"

void _start( void ) {
    kPrintln("Kenrel32 Initializing...");

    if (kIsSupport64() == FALSE) {
        kPrintErr("This CPU does not support 64bit mode.");
        while(1);
    }

    if (kInitMemory() == FALSE) {
        kPrintErr("Memory Initialization Failed.");
        while(1);
    }

    kInitPageTables();
    kPrintln("Kernel32 Initialized.");

    copyKernel64ImageTo2MB();
    kSwitchTo64();
}