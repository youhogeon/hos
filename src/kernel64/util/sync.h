#ifndef __SYNC_H__
#define __SYNC_H__

#include "../types.h"

#define MUTEX_INVALIDID 0xFFFFFFFFFFFFFFFF

#pragma pack(push, 1)

typedef struct kMutexStruct {
    // ID와 잠금을 수행한 횟수
    volatile QWORD qwID;
    volatile DWORD dwLockCount;

    // 잠금 플래그
    volatile BOOL bLockFlag;

    BYTE vbPadding[3];
} MUTEX;

#pragma pack(pop)

BOOL kLockForSystemData(void);
void kUnlockForSystemData(BOOL bInterruptFlag);

void kInitMutex(MUTEX* pstMutex);
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);

#endif /*__SYNC_H__*/
