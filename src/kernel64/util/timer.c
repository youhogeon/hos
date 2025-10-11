#include "timer.h"

volatile QWORD g_qwTickCount;

QWORD kGetTickCount(void) { return g_qwTickCount; }
void kIncreaseTickCount(void) { g_qwTickCount++; }