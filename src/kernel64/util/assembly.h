#include "../types.h"

static inline void outb(WORD port, BYTE val) { __asm__ volatile("outb %0, %1" ::"a"(val), "Nd"(port)); }

static inline BYTE inb(WORD port) {
    BYTE ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));

    return ret;
}

static inline void loadGDTR(void* gdtr) { __asm__ volatile("lgdt (%0)" ::"r"(gdtr)); }
static inline void loadIDTR(void* idtr) { __asm__ volatile("lidt (%0)" ::"r"(idtr)); }
static inline void loadTR(WORD tr) { __asm__ volatile("ltr %w0" ::"r"(tr)); }

static inline void sti(void) { __asm__ volatile("sti"); }
static inline void cli(void) { __asm__ volatile("cli"); }

static inline QWORD kReadRFLAGS(void) {
    QWORD qwRFLAGS;

    __asm__ volatile("pushfq\n"
                     "popq %0"
                     : "=r"(qwRFLAGS));

    return qwRFLAGS;
}

static BOOL kSetInterruptFlag(BOOL bEnableInterrupt) {
    QWORD qwRFLAGS;

    // 이전의 RFLAGS 레지스터 값을 읽은 뒤에 인터럽트 가능/불가 처리
    qwRFLAGS = kReadRFLAGS();
    if (bEnableInterrupt == TRUE) {
        sti();
    } else {
        cli();
    }

    // 이전 RFLAGS 레지스터의 IF 비트(비트 9)를 확인하여 이전의 인터럽트 상태를 반환
    if (qwRFLAGS & 0x0200) {
        return TRUE;
    }

    return FALSE;
}

void reloadCS(WORD selector);
void reloadDS(WORD selector);
QWORD kReadTSC(void);
void kHlt(void);
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource);