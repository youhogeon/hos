
#ifndef __K64SWITCH_H__
#define __K64SWITCH_H__

#include "types.h"

void kReadCPUID(DWORD dwEAX, DWORD* pdwEAX, DWORD* pdwEBX, DWORD* pdwECX, DWORD* pdwEDX);
void kSwitchTo64(void);

#endif /*__K64SWITCH_H__*/