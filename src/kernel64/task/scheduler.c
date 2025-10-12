#include "scheduler.h"
#include "../io/PIT.h"
#include "../memory/discriptor.h"
#include "../util/assembly.h"
#include "../util/list.h"
#include "../util/memory.h"
#include "../util/sync.h"
#include "../util/timer.h"
#include "switchcontext.h"
#include "tcbpool.h"

static SCHEDULER gs_stScheduler;

/**
 * 스케줄러를 초기화
 */
void kInitScheduler(void) {
    kInitTCBPool();

    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        kInitList(&(gs_stScheduler.vstReadyList[i]));
        gs_stScheduler.viExecuteCount[i] = 0;
    }
    kInitList(&(gs_stScheduler.stWaitList));

    gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
    gs_stScheduler.qwProcessorLoad = 0;

    // kernel task
    gs_stScheduler.pstRunningTask = kAllocateTCB();
    gs_stScheduler.pstRunningTask->qwFlags = TASK_FLAGS_HIGHEST;

    kInitPIT(MSTOCOUNT(1), 1);
}

/**
 * 현재 수행 중인 태스크를 설정
 */
void kSetRunningTask(TCB* pstTask) {
    BOOL bPreviousFlag = kLockForSystemData();
    gs_stScheduler.pstRunningTask = pstTask;
    kUnlockForSystemData(bPreviousFlag);
}

/**
 * 현재 수행 중인 태스크를 반환
 */
TCB* kGetRunningTask(void) {
    BOOL bPreviousFlag = kLockForSystemData();
    TCB* pstRunningTask = gs_stScheduler.pstRunningTask;
    kUnlockForSystemData(bPreviousFlag);

    return pstRunningTask;
}

/**
 * 태스크 리스트에서 다음으로 실행할 태스크를 얻음
 */
TCB* kGetNextTaskToRun(void) {
    // 높은 우선 순위에서 낮은 우선 순위까지 리스트를 확인하여 스케줄링할 태스크를 선택
    // 기아 현상을 방지하기 위해서 viExecuteCount가 iTaskCount보다 크면 다음 우선 순위의 태스크 선택
    //
    // 큐에 태스크가 있으나 모든 큐의 태스크가 1회씩 실행된 경우, 모든 큐가 프로세서를
    // 양보하여 태스크를 선택하지 못할 수 있으니 NULL일 경우 한번 더 수행

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
            int iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

            if (gs_stScheduler.viExecuteCount[i] < iTaskCount) {
                gs_stScheduler.viExecuteCount[i]++;

                return (TCB*)kRemoveListFromHeader(&(gs_stScheduler.vstReadyList[i]));
            } else {
                gs_stScheduler.viExecuteCount[i] = 0;
            }
        }
    }

    return NULL;
}

/**
 * 태스크를 스케줄러의 준비 리스트에 삽입
 */
BOOL kAddTaskToReadyList(TCB* pstTask) {
    BYTE bPriority = GETPRIORITY(pstTask->qwFlags);
    if (bPriority >= TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }

    kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);

    return TRUE;
}

/**
 * 태스크를 생성
 */
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    BOOL bPreviousFlag = kLockForSystemData();

    TCB* pstTask = kAllocateTCB();
    if (pstTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return NULL;
    }

    kUnlockForSystemData(bPreviousFlag);

    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    void* pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));

    // TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링될 수 있도록 함
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

    bPreviousFlag = kLockForSystemData();
    kAddTaskToReadyList(pstTask);
    kUnlockForSystemData(bPreviousFlag);

    return pstTask;
}

/**
 * 준비 큐에서 태스크를 제거
 */
TCB* kRemoveTaskFromReadyList(QWORD qwTaskID) {
    BYTE bPriority;

    if (GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT) {
        return NULL;
    }

    // TCB 풀에서 해당 태스크의 TCB를 찾아 실제로 ID가 일치하는가 확인
    TCB* pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
    if (pstTarget->stLink.qwID != qwTaskID) {
        return NULL;
    }

    // 태스크가 존재하는 준비 리스트에서 태스크 제거
    bPriority = GETPRIORITY(pstTarget->qwFlags);

    pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
    return pstTarget;
}

/**
 * 태스크의 우선 순위를 변경함
 */
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority) {
    if (bPriority > TASK_MAXREADYLISTCOUNT) {
        return FALSE;
    }

    BOOL bPreviousFlag = kLockForSystemData();

    // 현재 실행중인 태스크이면 우선 순위만 변경
    // PIT 컨트롤러의 인터럽트(IRQ 0)가 발생하여 태스크 전환이 수행될 때 변경된
    // 우선 순위의 리스트로 이동
    TCB* pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        SETPRIORITY(pstTarget->qwFlags, bPriority);

        kUnlockForSystemData(bPreviousFlag);
        return TRUE;
    }

    // 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 우선 순위를 설정
    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
        // 태스크 ID로 직접 찾아서 설정
        pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));

        if (pstTarget != NULL) {
            // 우선 순위를 설정
            SETPRIORITY(pstTarget->qwFlags, bPriority);
        }
    } else {
        // 우선 순위를 설정하고 준비 리스트에 다시 삽입
        SETPRIORITY(pstTarget->qwFlags, bPriority);
        kAddTaskToReadyList(pstTarget);
    }

    kUnlockForSystemData(bPreviousFlag);
    return TRUE;
}

/**
 * 다른 태스크를 찾아서 전환
 * 인터럽트나 예외가 발생했을 때 호출하면 안됨
 */
void kSchedule(void) {
    // 전환할 태스크가 있어야 함
    if (kGetReadyTaskCount() < 1) {
        return;
    }

    BOOL bPreviousFlag = kLockForSystemData();
    TCB* pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return;
    }

    // 현재 수행중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
    TCB* pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;

    // 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
    if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
    }

    // 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장할 필요가 없으므로, 대기 리스트에
    // 삽입하고 콘텍스트 전환
    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
        kSwitchContext(NULL, &(pstNextTask->stContext));
    } else {
        kAddTaskToReadyList(pstRunningTask);
        kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
    }

    // 프로세서 사용 시간을 업데이트
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    kUnlockForSystemData(bPreviousFlag);
}

/**
 * 인터럽트가 발생했을 때, 다른 태스크를 찾아 전환
 * 반드시 인터럽트나 예외가 발생했을 때 호출해야 함
 */
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char* pcContextAddress;

    BOOL bPreviousFlag = kLockForSystemData();

    // 전환할 태스크가 없으면 종료
    pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kUnlockForSystemData(bPreviousFlag);

        return FALSE;
    }

    // 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방법으로 태스크 전환
    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    // 현재 수행중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
    pstRunningTask = gs_stScheduler.pstRunningTask;
    gs_stScheduler.pstRunningTask = pstNextTask;
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    // 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가시킴
    if ((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE) {
        gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
    }

    if (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) {
        // 태스크 종료 플래그가 설정된 경우, 콘텍스트를 저장하지 않고 대기 리스트에만 삽입
        kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
    } else {
        // 태스크가 종료되지 않으면 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김

        kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
        kAddTaskToReadyList(pstRunningTask);
    }

    kUnlockForSystemData(bPreviousFlag);

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사해서
    // 자동으로 태스크 전환이 일어나도록 함
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    return TRUE;
}

/**
 * 프로세서를 사용할 수 있는 시간을 하나 줄임
 */
void kDecreaseProcessorTime(void) {
    if (gs_stScheduler.iProcessorTime > 0) {
        gs_stScheduler.iProcessorTime--;
    }
}

/**
 * 프로세서를 사용할 수 있는 시간이 다 되었는지 여부를 반환
 */
BOOL kIsProcessorTimeExpired(void) {
    if (gs_stScheduler.iProcessorTime <= 0) {
        return TRUE;
    }

    return FALSE;
}

/**
 * 태스크를 종료
 */
BOOL kEndTask(QWORD qwTaskID) {
    BOOL bPreviousFlag = kLockForSystemData();

    // 현재 실행중인 태스크이면 EndTask 비트를 설정하고 태스크를 전환
    TCB* pstTarget = gs_stScheduler.pstRunningTask;
    if (pstTarget->stLink.qwID == qwTaskID) {
        pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
        SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

        kUnlockForSystemData(bPreviousFlag);
        kSchedule();

        return TRUE;
    }

    // 준비 리스트에서 태스크를 찾음
    pstTarget = kRemoveTaskFromReadyList(qwTaskID);
    if (pstTarget == NULL) {
        kUnlockForSystemData(bPreviousFlag);
        return FALSE;
    }

    pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
    SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
    kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);

    kUnlockForSystemData(bPreviousFlag);

    return TRUE;
}

/**
 * 태스크가 자신을 종료함
 */
void kExitTask(void) { kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID); }

/**
 * 준비 큐에 있는 모든 태스크의 수를 반환
 */
int kGetReadyTaskCount(void) {
    BOOL bPreviousFlag = kLockForSystemData();

    int iTotalCount = 0;
    for (int i = 0; i < TASK_MAXREADYLISTCOUNT; i++) {
        iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
    }

    kUnlockForSystemData(bPreviousFlag);

    return iTotalCount;
}

/**
 * 전체 태스크의 수를 반환
 */
int kGetTaskCount(void) {
    int iTotalCount = kGetReadyTaskCount();

    BOOL bPreviousFlag = kLockForSystemData();
    iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;
    kUnlockForSystemData(bPreviousFlag);

    return iTotalCount;
}

/**
 * 프로세서의 사용률을 반환
 */
QWORD kGetProcessorLoad(void) { return gs_stScheduler.qwProcessorLoad; }

/**
 * 유휴 태스크
 * 대기 큐에 삭제 대기중인 태스크를 정리
 */
void kIdleTask(void) {
    // 프로세서 사용량 계산을 위해 기준 정보를 저장
    QWORD qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;
    QWORD qwLastMeasureTickCount = kGetTickCount();

    while (1) {
        // 현재 상태를 저장
        QWORD qwCurrentMeasureTickCount = kGetTickCount();
        QWORD qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

        // 프로세서 사용량을 계산
        // 100 - ( 유휴 태스크가 사용한 프로세서 시간 ) * 100 / ( 시스템 전체에서
        // 사용한 프로세서 시간 )
        if (qwCurrentMeasureTickCount - qwLastMeasureTickCount == 0) {
            gs_stScheduler.qwProcessorLoad = 0;
        } else {
            gs_stScheduler.qwProcessorLoad = 100 - (qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 /
                                                       (qwCurrentMeasureTickCount - qwLastMeasureTickCount);
        }

        // 현재 상태를 이전 상태에 보관
        qwLastMeasureTickCount = qwCurrentMeasureTickCount;
        qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

        // 프로세서의 부하에 따라 쉬게 함
        kHaltProcessorByLoad();

        // 대기 큐에 대기중인 태스크가 있으면 태스크를 종료함
        if (kGetListCount(&(gs_stScheduler.stWaitList)) >= 0) {
            while (1) {
                BOOL bPreviousFlag = kLockForSystemData();
                TCB* pstTask = kRemoveListFromHeader(&(gs_stScheduler.stWaitList));
                if (pstTask == NULL) {
                    kUnlockForSystemData(bPreviousFlag);
                    break;
                }
                kFreeTCB(pstTask->stLink.qwID);
                kUnlockForSystemData(bPreviousFlag);
            }
        }

        kSchedule();
    }
}

/**
 * 측정된 프로세서 부하에 따라 프로세서를 쉬게 함
 */
void kHaltProcessorByLoad(void) {
    if (gs_stScheduler.qwProcessorLoad < 40) {
        kHlt();
        kHlt();
        kHlt();
    } else if (gs_stScheduler.qwProcessorLoad < 80) {
        kHlt();
        kHlt();
    } else if (gs_stScheduler.qwProcessorLoad < 95) {
        kHlt();
    }
}
