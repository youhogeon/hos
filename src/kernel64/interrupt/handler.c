#include "handler.h"
#include "../io/keyboard.h"
#include "../io/video.h"
#include "../task/scheduler.h"
#include "../util/assembly.h"
#include "../util/timer.h"
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

void kTimerHandler(int iVectorNumber) {
    kSendEOIToPIC(iVectorNumber - PIC_IRQSTART_VECTOR);

    kIncreaseTickCount();
    kDecreaseProcessorTime();
    if (kIsProcessorTimeExpired() == TRUE) {
        kScheduleInInterrupt();
    }
}

void kKeyboardHandler(int iVectorNumber) {
    kGetKeyAndPutQueue();

    kSendEOIToPIC(iVectorNumber - PIC_IRQSTART_VECTOR);
}