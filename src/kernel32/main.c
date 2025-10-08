#include "memory.h"
#include "print.h"
#include "types.h"
#include "paging.h"

void _start( void ) {
    kPrintln("Kenrel32 Initializing...");

    if (kInitMemory() == FALSE) {
        kPrintln("Memory Initialization Failed.");
        goto end;
    }

    kInitPageTables();

    kPrintln("Kernel32 Initialized.");

    end:
    while(1) ;
}