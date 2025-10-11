#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "../types.h"
#include "../util/list.h"
#include "task.h"

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5 ms)
#define TASK_PROCESSORTIME 5

#pragma pack(push, 1)

typedef struct kSchedulerStruct {
    TCB* pstRunningTask;

    // 현재 수행 중인 태스크가 사용할 수 있는 프로세서 시간
    int iProcessorTime;

    // 실행할 태스크가 준비중인 리스트
    LIST stReadyList;
} SCHEDULER;

#pragma pack(pop)

void kInitScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun(void);
void kAddTaskToReadyList(TCB* pstTask);
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);

#endif /*__SCHEDULER_H__*/
