#include "PIT.h"
#include "../util/assembly.h"

void kInitPIT(WORD wCount, BOOL bPeriodic) {
    if (bPeriodic == TRUE) {
        outb(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    } else {
        outb(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);
    }

    // initial value
    outb(PIT_PORT_COUNTER0, wCount);
    outb(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0(void) {
    outb(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    BYTE bLowByte = inb(PIT_PORT_COUNTER0);
    BYTE bHighByte = inb(PIT_PORT_COUNTER0);

    WORD wTemp = bHighByte;
    wTemp = (wTemp << 8) | bLowByte;

    return wTemp;
}

/**
 * PIT를 이용하여 wCount만큼 대기
 * @param wCount 대기할 카운트 값 (최대 0xFFFF)
 */
void kWaitUsingDirectPIT(WORD wCount) {
    // PIT 컨트롤러를 0~0xFFFF까지 반복해서 카운팅하도록 설정
    kInitPIT(0, TRUE);

    WORD wLastCounter0 = kReadCounter0();
    while (1) {
        WORD wCurrentCounter0 = kReadCounter0();

        if (((wLastCounter0 - wCurrentCounter0) & 0xFFFF) >= wCount) {
            break;
        }
    }
}