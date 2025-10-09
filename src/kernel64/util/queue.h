#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "../types.h"

#pragma pack(push, 1)

typedef struct kQueueManagerStruct {
    int dataSize;
    int maxDataCount;

    void* pvQueueArray;
    int putIdx;
    int getIdx;

    BOOL isLastOperationPut;
} QUEUE;

#pragma pack(pop)

void kInitQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
BOOL kIsQueueFull(const QUEUE* pstQueue);
BOOL kIsQueueEmpty(const QUEUE* pstQueue);
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData);
BOOL kGetQueue(QUEUE* pstQueue, void* pvData);

#endif /*__QUEUE_H__*/