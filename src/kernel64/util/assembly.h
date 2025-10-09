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

void reloadCS(WORD selector);
void reloadDS(WORD selector);