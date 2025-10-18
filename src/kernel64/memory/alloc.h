#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "../types.h"
#include "memory_const.h"

#define DYNAMICMEMORY_START_ADDRESS MEMORY_ADDR_HEAP
#define DYNAMICMEMORY_MIN_SIZE (1 * 1024)

#define DYNAMICMEMORY_ALLOCATABLE 0x01
#define DYNAMICMEMORY_UNAVAILABLE 0x00

#pragma pack(push, 1)

typedef struct kBitmapStruct {
    BYTE* pbBitmap;
    QWORD qwExistBitCount;
} BITMAP;

typedef struct kDynamicMemoryManagerStruct {
    // 블록 리스트의 총 개수와 가장 크기가 가장 작은 블록의 개수, 그리고 할당된 메모리 크기
    int iMaxLevelCount;
    int iBlockCountOfSmallestBlock;
    QWORD qwUsedSize;

    // 블록 풀의 시작 어드레스와 마지막 어드레스
    QWORD qwStartAddress;
    QWORD qwEndAddress;

    // 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역과 비트맵 자료구조의 어드레스
    BYTE* pbAllocatedBlockListIndex;
    BITMAP* pstBitmapOfLevel;
} DYNAMICMEMORY;

#pragma pack(pop)

void kInitDynamicMemory(void);
void* kAllocateMemory(QWORD qwSize);
BOOL kFreeMemory(void* pvAddress);
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize,
                                  QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);

#endif /*__ALLOC_H__*/
