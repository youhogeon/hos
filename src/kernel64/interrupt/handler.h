#ifndef __INTERRUPTHANDLER_H__
#define __INTERRUPTHANDLER_H__

#include "../types.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);

void kTimerHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);

#endif /*__INTERRUPTHANDLER_H__*/