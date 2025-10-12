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

void kDeviceNotAvailableHandler(int iVectorNumber) {
    TCB *pstFPUTask, *pstCurrentTask;
    QWORD qwLastFPUTaskID;

    // CR0 컨트롤 레지스터의 TS 비트를 0으로 설정
    kClearTS();

    // 이전에 FPU를 사용한 태스크가 있는지 확인하여, 있다면 FPU의 상태를 태스크에 저장
    qwLastFPUTaskID = kGetLastFPUUsedTaskID();
    pstCurrentTask = kGetRunningTask();

    // 이전에 FPU를 사용한 것이 자신이면 아무것도 안 함
    if (qwLastFPUTaskID == pstCurrentTask->stLink.qwID) {
        return;
    } else if (qwLastFPUTaskID != TASK_INVALIDID) {
        pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
        if ((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)) {
            kSaveFPUContext(pstFPUTask->vqwFPUContext);
        }
    }

    // 현재 태스크가 FPU를 사용한 적이 있는 지 확인하여 FPU를 사용한 적이 없다면
    // 초기화하고, 사용한적이 있다면 저장된 FPU 콘텍스트를 복원
    if (pstCurrentTask->bFPUUsed == FALSE) {
        kInitFPU();
        pstCurrentTask->bFPUUsed = TRUE;
    } else {
        kLoadFPUContext(pstCurrentTask->vqwFPUContext);
    }

    // FPU를 사용한 태스크 ID를 현재 태스크로 변경
    kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

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