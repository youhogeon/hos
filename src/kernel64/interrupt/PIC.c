#include "PIC.h"
#include "../util/assembly.h"

void kInitPIC(void) {
    // init master PIC
    outb(PIC_MASTER_PORT1, 0x11);                // ICW1, IC4 비트(비트 0) = 1
    outb(PIC_MASTER_PORT2, PIC_IRQSTART_VECTOR); // ICW2, 인터럽트 벡터(0x20)
    outb(PIC_MASTER_PORT2, 0x04);                // ICW3, 슬레이브 PIC 연결 위치(0x04(비트 2))
    outb(PIC_MASTER_PORT2, 0x01);                // ICW4, uPM 비트(비트 0) = 1

    // init slave PIC
    outb(PIC_SLAVE_PORT1, 0x11);                    // ICW1, IC4 비트(비트 0) = 1
    outb(PIC_SLAVE_PORT2, PIC_IRQSTART_VECTOR + 8); // ICW2, 인터럽트 벡터(0x20 + 8)
    outb(PIC_SLAVE_PORT2, 0x02);                    // ICW3, 마스터 PIC 연결 위치(0x02(비트 1))
    outb(PIC_SLAVE_PORT2, 0x01);                    // ICW4, uPM 비트(비트 0) = 1
}

void kMaskPICInterrupt(WORD wIRQBitmask) {
    // 마스터 PIC 컨트롤러에 IMR 설정
    outb(PIC_MASTER_PORT2, (BYTE)wIRQBitmask); // OCW1, IRQ 0~7

    // 슬레이브 PIC 컨트롤러에 IMR 설정
    outb(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8)); // OCW1, IRQ 8~15
}

void kSendEOIToPIC(int iIRQNumber) {
    // send EOI to slave PIC
    if (iIRQNumber >= 8) {
        outb(PIC_SLAVE_PORT1, 0x20); // OCW2, EOI 비트(비트 5) = 1
    }

    outb(PIC_MASTER_PORT1, 0x20); // OCW2, EOI 비트(비트 5) = 1
}