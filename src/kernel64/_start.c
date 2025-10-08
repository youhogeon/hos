#include "io/keyboard.h"
#include "io/print.h"
#include "memory/discriptor.h"
#include "types.h"
#include "util/assembly.h"

void _start(void) {
    kPrintln("Switched to long mode.");

    kInitGDTAndTSS();
    kInitIDT();
    loadGDTR((GDTR*)GDTR_STARTADDRESS);
    loadTR(GDT_TSSSEGMENT);
    loadIDTR((IDTR*)IDTR_STARTADDRESS);

    if (kActivateKeyboard() == FALSE) {
        kPrintErr("Keyboard activation failed.");
        while (1)
            ;
    }

    kPrintln("Keyboard activated.");

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