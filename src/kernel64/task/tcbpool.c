#include "tcbpool.h"
#include "../memory/discriptor.h"
#include "../util/memory.h"

static TCBPOOLMANAGER gs_stTCBPoolManager;

void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize,
                QWORD exitAddress) {
    kMemSet(pstTCB->stContext.vqRegister, 0, sizeof(pstTCB->stContext.vqRegister));

    // 스택에 관련된 RSP, RBP 레지스터 설정
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

    *(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = exitAddress;

    // 세그먼트 셀렉터 설정
    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;
    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;

    // RIP, RFlags
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;
    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x0200; // IF 비트 설정 (인터럽트 활성화)

    // ID 및 스택, 그리고 플래그 저장
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}

/**
 * 태스크 풀 초기화
 */
void kInitTCBPool(void) {
    kMemSet(&(gs_stTCBPoolManager), 0, sizeof(gs_stTCBPoolManager));

    // 태스크 풀의 어드레스를 지정하고 초기화
    gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
    kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

    // TCB에 ID 할당
    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
    }

    // TCB의 최대 개수와 할당된 횟수를 초기화
    gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
    gs_stTCBPoolManager.iAllocatedCount = 1;
}

/**
 * TCB를 할당 받음
 */
TCB* kAllocateTCB(void) {
    if (gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount) {
        return NULL;
    }

    TCB* pstEmptyTCB;
    int i;

    for (i = 0; i < gs_stTCBPoolManager.iMaxCount; i++) {
        // ID의 상위 32비트가 0이면 할당되지 않은 TCB
        if ((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0) {
            pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
            break;
        }
    }

    // 상위 32비트를 0이 아닌 값으로 설정해서 할당된 TCB로 설정
    pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
    gs_stTCBPoolManager.iUseCount++;
    gs_stTCBPoolManager.iAllocatedCount++;
    if (gs_stTCBPoolManager.iAllocatedCount == 0) {
        gs_stTCBPoolManager.iAllocatedCount = 1;
    }

    return pstEmptyTCB;
}

/**
 * TCB를 해제함
 */
void kFreeTCB(QWORD qwID) {
    int i = GETTCBOFFSET(qwID);

    // TCB를 초기화하고 ID 설정
    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}

/**
 * TCB 풀에서 해당 오프셋의 TCB를 반환
 */
TCB* kGetTCBInTCBPool(int iOffset) {
    if ((iOffset < -1) && (iOffset > TASK_MAXCOUNT)) {
        return NULL;
    }

    return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

/**
 * 태스크가 존재하는지 여부를 반환
 */
BOOL kIsTaskExist(QWORD qwID) {
    TCB* pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
    // TCB가 없거나 ID가 일치하지 않으면 존재하지 않는 것임
    if ((pstTCB == NULL) || (pstTCB->stLink.qwID != qwID)) {
        return FALSE;
    }
    return TRUE;
}

/**
 *  스레드가 소속된 프로세스를 반환
 */
TCB* kGetProcessByThread(TCB* pstThread) {
    if (pstThread->qwFlags & TASK_FLAGS_PROCESS) {
        return pstThread;
    }

    TCB* pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

    if ((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)) {
        return NULL;
    }

    return pstProcess;
}