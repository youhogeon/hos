#include "handler.h"
#include "../util/assembly.h"
#include "../util/print.h"
#include "PIC.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode) {
    kPrintErr("Exception Occur: ");

    char vcBuffer[3] = {
        '0' + iVectorNumber / 10,
        '0' + iVectorNumber % 10,
        0,
    };

    kPrintErr(vcBuffer);

    while (1) {
    }
}

void kCommonInterruptHandler(int iVectorNumber) { kSendEOIToPIC(iVectorNumber - PIC_IRQSTART_VECTOR); }

void kKeyboardHandler(int iVectorNumber) {
    kPrint("Keyboard: ");
    BYTE sc = inb(0x60);

    char vcBuffer[4] = {
        '0' + sc / 100,
        '0' + (sc / 10) % 10,
        '0' + sc % 10,
        0,
    };

    kPrintln(vcBuffer);

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTART_VECTOR);
}