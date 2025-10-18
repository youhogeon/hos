#include "sync.h"
#include "../task/scheduler.h"
#include "assembly.h"

/**
 * 시스템 전역변수 위한 잠금 함수
 */
BOOL kLockForSystemData(void) { return kSetInterruptFlag(FALSE); }

/**
 * 시스템 전역변수 위한 잠금 해제 함수
 */
void kUnlockForSystemData(BOOL bInterruptFlag) { kSetInterruptFlag(bInterruptFlag); }

/**
 * 뮤텍스를 초기화
 */
void kInitMutex(MUTEX* pstMutex) {
    pstMutex->bLockFlag = FALSE;
    pstMutex->dwLockCount = 0;
    pstMutex->qwID = MUTEX_INVALIDID;
}

/**
 * 태스크 사이에서 사용하는 데이터를 위한 잠금 함수
 */
void kLock(MUTEX* pstMutex) {
    QWORD qwID = kGetRunningTask()->stLink.qwID;

    if (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
        if (pstMutex->qwID == qwID) {
            pstMutex->dwLockCount++;
            return;
        }

        while (kTestAndSet(&(pstMutex->bLockFlag), 0, 1) == FALSE) {
            kSchedule();
        }
    }

    pstMutex->dwLockCount = 1;
    pstMutex->qwID = qwID;
}

/**
 * 태스크 사이에서 사용하는 데이터를 위한 잠금 해제 함수
 */
void kUnlock(MUTEX* pstMutex) {
    QWORD qwID = kGetRunningTask()->stLink.qwID;

    // 뮤텍스를 잠근 태스크가 아니면 실패
    if ((pstMutex->bLockFlag == FALSE) || (pstMutex->qwID != qwID)) {
        return;
    }

    if (pstMutex->dwLockCount > 1) {
        pstMutex->dwLockCount--;
        return;
    }

    pstMutex->qwID = MUTEX_INVALIDID;
    pstMutex->dwLockCount = 0;
    pstMutex->bLockFlag = FALSE;
}
