#include "scheduler.h"
#include "../io/PIT.h"
#include "../memory/discriptor.h"
#include "../util/assembly.h"
#include "../util/list.h"
#include "../util/memory.h"
#include "switchcontext.h"
#include "tcbpool.h"

static SCHEDULER gs_stScheduler;

/**
 * 스케줄러를 초기화
 */
void kInitScheduler(void) {
    kInitTCBPool();
    kInitList(&(gs_stScheduler.stReadyList));

    // kernel task
    gs_stScheduler.pstRunningTask = kAllocateTCB();

    kInitPIT(MSTOCOUNT(1), 1);
}

/**
 * 현재 수행 중인 태스크를 설정
 */
void kSetRunningTask(TCB* pstTask) { gs_stScheduler.pstRunningTask = pstTask; }

/**
 * 현재 수행 중인 태스크를 반환
 */
TCB* kGetRunningTask(void) { return gs_stScheduler.pstRunningTask; }

/**
 * 태스크 리스트에서 다음으로 실행할 태스크를 얻음
 */
TCB* kGetNextTaskToRun(void) {
    if (kGetListCount(&(gs_stScheduler.stReadyList)) == 0) {
        return NULL;
    }

    return (TCB*)kRemoveListFromHeader(&(gs_stScheduler.stReadyList));
}

/**
 * 태스크를 스케줄러의 준비 리스트에 삽입
 */
void kAddTaskToReadyList(TCB* pstTask) { kAddListToTail(&(gs_stScheduler.stReadyList), pstTask); }

/**
 * 태스크를 생성
 */
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress) {
    TCB* pstTask = kAllocateTCB();
    if (pstTask == NULL) {
        return NULL;
    }

    // 태스크 ID로 스택 어드레스 계산, 하위 32비트가 스택 풀의 오프셋 역할 수행
    void* pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));

    // TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링될 수 있도록 함
    kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);
    kAddTaskToReadyList(pstTask);

    return pstTask;
}

/**
 * 다른 태스크를 찾아서 전환
 * 인터럽트나 예외가 발생했을 때 호출하면 안됨
 */
void kSchedule(void) {
    // 전환할 태스크가 있어야 함
    if (kGetListCount(&(gs_stScheduler.stReadyList)) == 0) {
        return;
    }

    // 전환하는 도중 인터럽트가 발생하여 태스크 전환이 또 일어나면 곤란하므로 전환하는
    // 동안 인터럽트가 발생하지 못하도록 설정
    BOOL bPreviousFlag = kSetInterruptFlag(FALSE);
    // 실행할 다음 태스크를 얻음
    TCB* pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        kSetInterruptFlag(bPreviousFlag);
        return;
    }

    TCB* pstRunningTask = gs_stScheduler.pstRunningTask;
    kAddTaskToReadyList(pstRunningTask);

    // 다음 태스크를 현재 수행 중인 태스크로 설정한 후 콘텍스트 전환
    gs_stScheduler.pstRunningTask = pstNextTask;
    kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));

    // 프로세서 사용 시간을 업데이트
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

    kSetInterruptFlag(bPreviousFlag);
}

/**
 * 인터럽트가 발생했을 때, 다른 태스크를 찾아 전환
 * 반드시 인터럽트나 예외가 발생했을 때 호출해야 함
 */
BOOL kScheduleInInterrupt(void) {
    TCB *pstRunningTask, *pstNextTask;
    char* pcContextAddress;

    // 전환할 태스크가 없으면 종료
    pstNextTask = kGetNextTaskToRun();
    if (pstNextTask == NULL) {
        return FALSE;
    }

    // 테스크 전환
    // 인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어씀
    pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

    // 현재 태스크를 얻어서 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김
    pstRunningTask = gs_stScheduler.pstRunningTask;
    kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
    kAddTaskToReadyList(pstRunningTask);

    // 전환해서 실행할 태스크를 Running Task로 설정하고 콘텍스트를 IST에 복사해서
    // 자동으로 태스크 전환이 일어나도록 함
    gs_stScheduler.pstRunningTask = pstNextTask;
    kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

    // 프로세서 사용 시간을 업데이트
    gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;
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
