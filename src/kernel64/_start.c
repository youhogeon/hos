#include "interrupt/PIC.h"
#include "io/keyboard.h"
#include "io/video.h"
#include "memory/discriptor.h"
#include "types.h"
#include "util/assembly.h"

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
        kPrintErr("Keyboard initialization failed.");
        return;
    }

    kPrintln("Keyboard initialized.");

    // 마무리
    sti();
    kPrintln("Kernel64 initialized.");
    kClear(5);
    kPrintln(" ");

    while (1) {
        KEYDATA stData;
        if (kGetKeyFromKeyQueue(&stData) == FALSE) {
            continue;
        }

        if (stData.bFlags & KEY_FLAGS_DOWN) {
            char key[2] = {
                stData.bASCIICode,
                0,
            };
            kPrint(key);
        }
    }
}

void _initMemory(void) {
    kInitGDTAndTSS();
    kInitIDT();

    loadGDTR((GDTR*)GDTR_STARTADDRESS);
    loadTR(GDT_TSSSEGMENT);
    loadIDTR((IDTR*)IDTR_STARTADDRESS);

    reloadCS(GDT_KERNELCODESEGMENT);
    reloadDS(GDT_KERNELDATASEGMENT);
}