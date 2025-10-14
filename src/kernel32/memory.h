#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "types.h"

#define MEMORY_REQUIREMENT 0x4000000 // 64MB
#define MEMORY_K64_SOURCE 0x11000
#define MEMORY_PAGE_BEGIN 0x100000
#define MEMORY_K64_BEGIN 0x200000
#define MEMORY_K64_SIZE 0x400000

BOOL kInitMemory(void);
void copyKernel64ImageTo2MB(void);

#endif /*__MEMORY_H__*/