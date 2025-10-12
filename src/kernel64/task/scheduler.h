#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "../types.h"
#include "../util/list.h"
#include "tcbpool.h"

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5 ms)
#define TASK_PROCESSORTIME 5
// 준비 리스트의 수
#define TASK_MAXREADYLISTCOUNT 5

#pragma pack(push, 1)

typedef struct kSchedulerStruct {
    TCB* pstRunningTask;

    int iProcessorTime;

    // 실행할 태스크가 준비중인 리스트, 태스크의 우선 순위에 따라 구분
    LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

    // 종료할 태스크가 대기중인 리스트
    LIST stWaitList;

    // 각 우선 순위별로 태스크를 실행한 횟수를 저장하는 자료구조
    int viExecuteCount[TASK_MAXREADYLISTCOUNT];

    // 프로세서 부하를 계산하기 위한 자료구조
    QWORD qwProcessorLoad;

    // 유휴 태스크(Idle Task)에서 사용한 프로세서 시간
    QWORD qwSpendProcessorTimeInIdleTask;

    // 마지막으로 FPU를 사용한 태스크의 ID
    QWORD qwLastFPUUsedTaskID;
} SCHEDULER;

#pragma pack(pop)

void kInitScheduler(void);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun(void);
BOOL kAddTaskToReadyList(TCB* pstTask);
TCB* kRemoveTaskFromReadyList(QWORD qwTaskID);
BOOL kChangePriority(QWORD qwID, BYTE bPriority);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask(void);

void kSchedule(void);
BOOL kScheduleInInterrupt(void);

int kGetReadyTaskCount(void);
int kGetTaskCount(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
QWORD kGetProcessorLoad(void);

void kIdleTask(void);
void kHaltProcessorByLoad(void);

QWORD kGetLastFPUUsedTaskID(void);
void kSetLastFPUUsedTaskID(QWORD qwTaskID);

#endif /*__SCHEDULER_H__*/
