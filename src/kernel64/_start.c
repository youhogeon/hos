#include "types.h"
#include "io/print.h"
#include "io/keyboard.h"

void _start( void ) {
    kPrintln("Switched to long mode.");

    if (kActivateKeyboard() == FALSE) {
        kPrintErr("Keyboard activation failed.");
        while (1);
    }

    kPrintln("Keyboard activated.");

    kPrintln("Kernel64 initialized.");

    while(1) {
        if (kIsOutputBufferFull() == FALSE) {
            continue;
        }

        BYTE scanCode = kGetKeyboardScanCode();
        BYTE flag;
        char key[2] = {0, };

        if (kConvertScanCodeToASCIICode(scanCode, (BYTE*)&key[0], &flag) == TRUE) {
            if ((flag & KEY_FLAGS_DOWN) == 0) {
                continue;
            }

            kPrint(key);
        }

    }
}