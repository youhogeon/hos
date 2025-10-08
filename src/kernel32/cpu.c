#include "cpu.h"
#include "k64switch.h"

BOOL kIsSupport64(void) {
    DWORD dwEAX, dwEBX, dwECX, dwEDX;
    kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
    
    return (dwEDX & ( 1 << 29 )) != 0;
}