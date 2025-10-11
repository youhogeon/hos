#ifndef __TCBPOOL_H__
#define __TCBPOOL_H__

#include "../types.h"
#include "../util/list.h"
#include "task.h"

// 태스크 풀의 어드레스
#define TASK_TCBPOOLADDRESS 0x800000
#define TASK_MAXCOUNT 1024

// 스택 풀과 스택의 크기
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE (1024 * 8)

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

#pragma pack(push, 1)

// TCB 풀의 상태를 관리하는 자료구조
typedef struct kTCBPoolManagerStruct {
    // 태스크 풀에 대한 정보
    TCB* pstStartAddress;
    int iMaxCount;
    int iUseCount;

    // TCB가 할당된 횟수
    int iAllocatedCount;
} TCBPOOLMANAGER;

#pragma pack(pop)

void kInitTCBPool(void);
TCB* kAllocateTCB(void);
void kFreeTCB(QWORD qwID);

#endif /*__TCBPOOL_H__*/
