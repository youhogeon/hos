#include "interrupt/PIC.h"
#include "io/keyboard.h"
#include "io/video.h"
#include "memory/discriptor.h"
#include "shell/shell.h"
#include "types.h"
#include "util/assembly.h"
#include "util/memory.h"

void _initMemory(void);

void _start(void) {
    kPrintln("Switched to long mode.");

    // Init memory
    _initMemory();
    kPrintln("Memory initialized.");

    // Init PIC, keyboard
    kInitPIC();
    kMaskPICInterrupt(0);

    if (kInitKeyboard() == FALSE) {
        kPrintErr("PIC initialization failed.");
        return;
    }

    kPrintln("PIC initialized.");

    // 마무리
    sti();
    kPrintln("Kernel64 initialized.");
    kClear(5);
    kPrintln("");

    kStartConsoleShell();
}

void _initMemory(void) {
    kMemSize();

    kInitGDTAndTSS();
    kInitIDT();

    loadGDTR((GDTR*)GDTR_STARTADDRESS);
    loadTR(GDT_TSSSEGMENT);
    loadIDTR((IDTR*)IDTR_STARTADDRESS);

    reloadCS(GDT_KERNELCODESEGMENT);
    reloadDS(GDT_KERNELDATASEGMENT);
}