#include "interrupt/PIC.h"
#include "io/keyboard.h"
#include "memory/discriptor.h"
#include "types.h"
#include "util/assembly.h"
#include "util/print.h"

void _initMemory(void);

void _start(void) {
    kPrintln("Switched to long mode.");

    _initMemory();
    kPrintln("Memory Initialized.");

    kInitPIC();
    kMaskPICInterrupt(0);

    if (kActivateKeyboard() == FALSE) {
        kPrintErr("Keyboard activation failed.");
        return;
    }

    kPrintln("Keyboard activated.");

    sti();
    kPrintln("Kernel64 initialized.");

    while (1) {
        if (kIsOutputBufferFull() == FALSE) {
            continue;
        }

        BYTE scanCode = kGetKeyboardScanCode();
        BYTE flag;
        char key[2] = {
            0,
        };

        if (scanCode == 28) {
            scanCode = 0 / 0;
        }

        if (kConvertScanCodeToASCIICode(scanCode, (BYTE*)&key[0], &flag) == TRUE) {
            if ((flag & KEY_FLAGS_DOWN) == 0) {
                continue;
            }

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