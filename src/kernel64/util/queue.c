#include "queue.h"
#include "memory.h"

void kInitQueue(QUEUE* pstQueue, void* pvQueueBuffer, int maxDataCount, int dataSize) {
    pstQueue->maxDataCount = maxDataCount;
    pstQueue->dataSize = dataSize;
    pstQueue->pvQueueArray = pvQueueBuffer;

    pstQueue->putIdx = 0;
    pstQueue->getIdx = 0;
    pstQueue->isLastOperationPut = FALSE;
}

BOOL kIsQueueFull(const QUEUE* pstQueue) {
    if ((pstQueue->getIdx == pstQueue->putIdx) && (pstQueue->isLastOperationPut == TRUE)) {
        return TRUE;
    }

    return FALSE;
}

BOOL kIsQueueEmpty(const QUEUE* pstQueue) {
    if ((pstQueue->getIdx == pstQueue->putIdx) && (pstQueue->isLastOperationPut == FALSE)) {
        return TRUE;
    }

    return FALSE;
}

BOOL kPutQueue(QUEUE* pstQueue, const void* pvData) {
    if (kIsQueueFull(pstQueue) == TRUE) {
        return FALSE;
    }

    kMemCpy((char*)pstQueue->pvQueueArray + (pstQueue->dataSize * pstQueue->putIdx), pvData, pstQueue->dataSize);

    pstQueue->putIdx = (pstQueue->putIdx + 1) % pstQueue->maxDataCount;
    pstQueue->isLastOperationPut = TRUE;

    return TRUE;
}

BOOL kGetQueue(QUEUE* pstQueue, void* pvData) {
    if (kIsQueueEmpty(pstQueue) == TRUE) {
        return FALSE;
    }

    kMemCpy(pvData, (char*)pstQueue->pvQueueArray + (pstQueue->dataSize * pstQueue->getIdx), pstQueue->dataSize);

    pstQueue->getIdx = (pstQueue->getIdx + 1) % pstQueue->maxDataCount;
    pstQueue->isLastOperationPut = FALSE;

    return TRUE;
}