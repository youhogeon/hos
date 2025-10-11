#include "tcbpool.h"
#include "../memory/discriptor.h"
#include "../util/memory.h"

static TCBPOOLMANAGER gs_stTCBPoolManager;

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
    // 태스크 ID의 하위 32비트가 인덱스 역할을 함
    int i = qwID & 0xFFFFFFFF;

    // TCB를 초기화하고 ID 설정
    kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
    gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

    gs_stTCBPoolManager.iUseCount--;
}