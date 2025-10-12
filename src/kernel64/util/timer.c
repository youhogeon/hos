#include "timer.h"
#include "../task/scheduler.h"

static volatile QWORD g_qwTickCount;
static volatile QWORD gs_qwRandomValue = 0;

QWORD kGetTickCount(void) { return g_qwTickCount; }
void kIncreaseTickCount(void) { g_qwTickCount++; }

void kSleep(QWORD qwMillisecond) {
    QWORD qwLastTickCount;

    qwLastTickCount = g_qwTickCount;

    while ((g_qwTickCount - qwLastTickCount) <= qwMillisecond) {
        kSchedule();
    }
}

QWORD kRandom(void) {
    gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
    return gs_qwRandomValue;
}