#ifndef __TIMER_H__
#define __TIMER_H__

#include "../types.h"

QWORD kGetTickCount(void);
void kIncreaseTickCount(void);
void kSleep(QWORD qwMillisecond);
QWORD kRandom(void);

#endif /*__TIMER_H__*/