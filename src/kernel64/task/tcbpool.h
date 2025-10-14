#ifndef __TCBPOOL_H__
#define __TCBPOOL_H__

#include "../memory/memory_const.h"
#include "../types.h"
#include "../util/list.h"

// 태스크 풀의 어드레스
#define TASK_TCBPOOLADDRESS MEMORY_ADDR_TCBPOOL
#define TASK_MAXCOUNT 1024

// 스택 풀과 스택의 크기
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + sizeof(TCB) * TASK_MAXCOUNT)
#define TASK_STACKSIZE (1024 * 8)

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID 0xFFFFFFFFFFFFFFFF

// SS, RSP, RFLAGS, CS, RIP + ISR에서 저장하는 19개의 레지스터
#define TASK_REGISTERCOUNT (5 + 19)
#define TASK_REGISTERSIZE 8

// Context 자료구조의 레지스터 오프셋
#define TASK_GSOFFSET 0
#define TASK_FSOFFSET 1
#define TASK_ESOFFSET 2
#define TASK_DSOFFSET 3
#define TASK_R15OFFSET 4
#define TASK_R14OFFSET 5
#define TASK_R13OFFSET 6
#define TASK_R12OFFSET 7
#define TASK_R11OFFSET 8
#define TASK_R10OFFSET 9
#define TASK_R9OFFSET 10
#define TASK_R8OFFSET 11
#define TASK_RSIOFFSET 12
#define TASK_RDIOFFSET 13
#define TASK_RDXOFFSET 14
#define TASK_RCXOFFSET 15
#define TASK_RBXOFFSET 16
#define TASK_RAXOFFSET 17
#define TASK_RBPOFFSET 18
#define TASK_RIPOFFSET 19
#define TASK_CSOFFSET 20
#define TASK_RFLAGSOFFSET 21
#define TASK_RSPOFFSET 22
#define TASK_SSOFFSET 23

// flags
#define TASK_FLAGS_ENDTASK 0x8000000000000000
#define TASK_FLAGS_SYSTEM 0x4000000000000000
#define TASK_FLAGS_PROCESS 0x2000000000000000
#define TASK_FLAGS_THREAD 0x1000000000000000
#define TASK_FLAGS_IDLE 0x0800000000000000

#define TASK_FLAGS_HIGHEST 0
#define TASK_FLAGS_HIGH 1
#define TASK_FLAGS_MEDIUM 2
#define TASK_FLAGS_LOW 3
#define TASK_FLAGS_LOWEST 4
#define TASK_FLAGS_WAIT 0xFF

#define GETPRIORITY(x) ((x) & 0xFF)
#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))
#define GETTCBOFFSET(x) ((x) & 0xFFFFFFFF)
#define GETTCBFROMTHREADLINK(x) (TCB*)((QWORD)(x) - offsetof(TCB, stThreadLink))

#pragma pack(push, 1)
typedef struct kContextStruct {
    QWORD vqRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct {
    LISTLINK stLink;

    QWORD qwFlags;
    void* pvMemoryAddress;
    QWORD qwMemorySize;

    // Thread data
    QWORD qwParentProcessID;
    QWORD vqwFPUContext[512 / 8];
    CONTEXT stContext;

    LISTLINK stThreadLink;
    LIST stChildThreadList;

    void* pvStackAddress;
    QWORD qwStackSize;

    BOOL bFPUUsed;

    char vcPadding[11];
} TCB;

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

void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize,
                QWORD exitAddress);

void kInitTCBPool(void);
TCB* kAllocateTCB(void);
void kFreeTCB(QWORD qwID);
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwID);
TCB* kGetProcessByThread(TCB* pstThread);

#endif /*__TCBPOOL_H__*/
