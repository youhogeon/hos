#include "fs/mintfs.h"
#include "interrupt/PIC.h"
#include "io/ATA.h"
#include "io/keyboard.h"
#include "io/video.h"
#include "memory/alloc.h"
#include "memory/discriptor.h"
#include "shell/shell.h"
#include "task/scheduler.h"
#include "types.h"
#include "util/assembly.h"
#include "util/memory.h"

void _initMemory(void) {
    kMemSize();

    kInitGDTAndTSS();
    kInitIDT();

    loadGDTR((GDTR*)GDTR_STARTADDRESS);
    loadTR(GDT_TSSSEGMENT);
    loadIDTR((IDTR*)IDTR_STARTADDRESS);

    reloadCS(GDT_KERNELCODESEGMENT);
    reloadDS(GDT_KERNELDATASEGMENT);

    kInitDynamicMemory();

    kPrintln("Memory initialized.");
}

void _initDisk(void) {
    if (kInitATA() == FALSE) {
        kPrintErr("ATA HDD initialization failed.");
        return;
    }

    kPrintln("ATA HDD initialized.");

    if (kInitFileSystem() == FALSE) {
        kPrintErr("File System initialization failed.");
        return;
    }

    kPrintln("File System initialized.");
}

void _start(void) {
    kPrintln("Switched to long mode.");

    // Init memory
    _initMemory();

    // Init scheduler
    kInitScheduler();
    kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
    kPrintln("Scheduler initialized.");

    // Init PIC, keyboard
    kInitPIC();
    kMaskPICInterrupt(0);

    if (kInitKeyboard() == FALSE) {
        kPrintErr("PIC initialization failed.");
        return;
    }

    sti();
    kPrintln("PIC initialized.");

    // Init Disk, File system
    _initDisk();

    // 마무리
    kPrintln("Kernel64 initialized.");
    kClear(5);
    kPrintln("");

    kStartConsoleShell();
}