
#include "shell.h"
#include "../io/ATA.h"
#include "../io/PIT.h"
#include "../io/RTC.h"
#include "../io/keyboard.h"
#include "../io/video.h"
#include "../memory/alloc.h"
#include "../task/scheduler.h"
#include "../task/tcbpool.h"
#include "../util/assembly.h"
#include "../util/memory.h"
#include "../util/string.h"
#include "../util/sync.h"
#include "../util/timer.h"

////////////////////////////////////////////////////////////////
// Shell Commands
////////////////////////////////////////////////////////////////

static void kHelp(PARAMETER_LIST* pstList);

static void kCls(PARAMETER_LIST* pstList) { kClear(0); }

static void kEcho(PARAMETER_LIST* pstList) {
    char pcParameter[100];
    int iLength;

    while (1) {
        iLength = kGetNextParameter(pstList, pcParameter);
        if (iLength == 0) {
            break;
        }
        kPrintln(pcParameter);
    }
}

static void kReboot_(PARAMETER_LIST* pstList) { kReboot(); }

static void kShowMemoryInfo(PARAMETER_LIST* pstList) {
    QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

    kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

    DWORD totalMem = kMemSize();
    QWORD systemUsed = qwStartAddress + qwMetaSize;
    QWORD freeMem = totalMem - systemUsed - qwUsedSize;

    kPrintln("======== Memory Information ========");
    kPrintf("Total size:    [%d] MB\n", totalMem / 1024 / 1024);
    kPrintf("System used:   [%d] MB\n", systemUsed / 1024 / 1024);
    kPrintf("Dynamic used:  [%d] MB\n", qwUsedSize / 1024 / 1024);
    kPrintf("Free size:     [%d] MB\n", freeMem / 1024 / 1024);
}

static void kWaitUsingPIT(PARAMETER_LIST* pstList) {
    char vcParameter[100];
    int iLength;

    int i;

    // 파라미터 초기화
    if (kGetNextParameter(pstList, vcParameter) == 0) {
        kPrintln("[Usage] wait <ms>");
        return;
    }

    long lMillisecond = kAToI(vcParameter, 10);
    kPrintf("%d ms sleep...\n", lMillisecond);

    kSleep((QWORD)lMillisecond);

    kPrintf("%d ms sleep complete\n", lMillisecond);
    kInitPIT(MSTOCOUNT(1), TRUE);
}

static void kMeasureProcessorSpeed(PARAMETER_LIST* pstList) {
    int i;
    QWORD qwLastTSC, qwTotalTSC = 0;

    kPrintf("Measuring.");

    cli();
    for (i = 0; i < 200; i++) {
        qwLastTSC = kReadTSC();
        kWaitUsingDirectPIT(MSTOCOUNT(50));
        qwTotalTSC += kReadTSC() - qwLastTSC;

        kPrintf(".");
    }
    kInitPIT(MSTOCOUNT(1), TRUE);
    sti();

    kPrintf("\nCPU speed: %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kCPULoad(PARAMETER_LIST* pstList) { kPrintf("Processor Load: %d%%\n", kGetProcessorLoad()); }

static void kShowDateAndTime(PARAMETER_LIST* pstList) {
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("%d/%d/%d (%s) ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("%d:%d:%d\n", bHour, bMinute, bSecond);
}

static void kTestTask1(void) {
    BYTE bData;
    int i = 0, iX = 0, iY = 0, iMargin;
    TCB* pstRunningTask;

    // 자신의 ID를 얻어서 화면 오프셋으로 사용
    pstRunningTask = kGetRunningTask();
    iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

    while (1) {
        switch (i) {
        case 0:
            iX++;
            if (iX >= (VGA_COLS - iMargin)) {
                i = 1;
            }
            break;

        case 1:
            iY++;
            if (iY >= (VGA_ROWS - iMargin)) {
                i = 2;
            }
            break;

        case 2:
            iX--;
            if (iX < iMargin) {
                i = 3;
            }
            break;

        case 3:
            iY--;
            if (iY < iMargin) {
                i = 0;
            }
            break;
        }

        char d[2] = {bData, 0};
        kPrintAt(iX, iY, d);
        bData++;

        kSchedule();
    }
}

static void kTestTask2(void) {
    char vcData[4] = {'-', '\\', '|', '/'};
    int i = 0;

    TCB* pstRunningTask = kGetRunningTask();
    int id = (pstRunningTask->stLink.qwID & 0xFFFFFFFF);

    while (1) {
        char d[2] = {vcData[i % 4], 0};
        kPrintAt(id % VGA_COLS, (id / VGA_COLS) % VGA_ROWS, d);

        i++;

        kSchedule();
    }
}

static void kCreateTestTask(PARAMETER_LIST* pstList) {
    char vcType[30];
    char vcCount[30];

    kGetNextParameter(pstList, vcType);
    kGetNextParameter(pstList, vcCount);

    int count = kAToI(vcCount, 10);
    if (count <= 0) {
        kPrintln("[Usage] createtask <type:1,2> <count>");
        return;
    }

    switch (kAToI(vcType, 10)) {
    case 1:
        for (int i = 0; i < count; i++) {
            if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL) {
                break;
            }
        }

        kPrintf("Task1 Created\n");
        break;

    case 2:
        for (int i = 0; i < count; i++) {
            if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL) {
                break;
            }
        }

        kPrintf("Task2 Created\n");
        break;
    default:
        kPrintln("[Usage] createtask <type:1,2> <count>");
    }
}

static void kChangeTaskPriority(PARAMETER_LIST* pstList) {
    char vcID[30];
    char vcPriority[30];
    QWORD qwID;

    kGetNextParameter(pstList, vcID);
    kGetNextParameter(pstList, vcPriority);

    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    } else {
        qwID = kAToI(vcID, 10);
    }

    if (qwID == 0) {
        kPrintln("[Usage] changepriority <ID> <PRIORITY:0,1,2,3,4)>");
        return;
    }

    BYTE bPriority = kAToI(vcPriority, 10);

    kPrintf("Change priority of task [0x%q] had changed to [%d]: ", qwID, bPriority);

    if (kChangePriority(qwID, bPriority) == TRUE) {
        kPrintlnColor("Success", VGA_ATTR_FOREGROUND_BRIGHTGREEN);
    } else {
        kPrintlnColor("Fail", VGA_ATTR_FOREGROUND_BRIGHTRED);
    }
}

static void kShowTaskList(PARAMETER_LIST* pstList) {
    int iCount = 0;

    kPrintf("Total Tasks: %d\n\n", kGetTaskCount());
    for (int i = 0; i < TASK_MAXCOUNT; i++) {
        TCB* pstTCB = kGetTCBInTCBPool(i);
        if ((pstTCB->stLink.qwID >> 32) == 0) {
            continue;
        }

        if ((iCount != 0) && ((iCount % 10) == 0)) {
            kPrintf("Press any key to continue... ('q' to quit) : ");

            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }

            kPrintf("\n");
        }

        kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Threads[%d]\n",
                1 + iCount++,
                pstTCB->stLink.qwID,
                GETPRIORITY(pstTCB->qwFlags),
                pstTCB->qwFlags,
                kGetListCount(&(pstTCB->stChildThreadList)));
        kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n",
                pstTCB->qwParentProcessID,
                pstTCB->pvMemoryAddress,
                pstTCB->qwMemorySize);
    }
}

static void kKillTask(PARAMETER_LIST* pstList) {
    char vcID[30];
    QWORD qwID;

    if (kGetNextParameter(pstList, vcID) == 0) {
        kPrintln("[Usage] killtask <ID>");
        kPrintln("[Usage] killtask -1 (for kill all tasks)");
        return;
    }

    // 태스크를 종료
    if (kMemCmp(vcID, "0x", 2) == 0) {
        qwID = kAToI(vcID + 2, 16);
    } else {
        qwID = kAToI(vcID, 10);
    }

    if (qwID == (QWORD)-1) {
        int iCount = 0;

        for (int i = 2; i < TASK_MAXCOUNT; i++) {
            TCB* pstTCB = kGetTCBInTCBPool(i);

            if (((pstTCB->stLink.qwID >> 32) == 0) || ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) != 0x00)) {
                continue;
            }

            if (kEndTask(pstTCB->stLink.qwID) == TRUE) {
                iCount++;
            }
        }

        kPrintf("Total %d tasks are killed.\n", iCount);
        return;
    }

    TCB* pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));

    if (((pstTCB->stLink.qwID >> 32) == 0) || ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) != 0x00)) {
        kPrintlnColor("Cannot kill system task", VGA_ATTR_FOREGROUND_BRIGHTYELLOW);
        return;
    }

    kPrintf("Kill Task [0x%q]: ", qwID);
    if (kEndTask(qwID) == TRUE) {
        kPrintlnColor("Success", VGA_ATTR_FOREGROUND_BRIGHTGREEN);
    } else {
        kPrintlnColor("Fail", VGA_ATTR_FOREGROUND_BRIGHTRED);
    }
}

static void kDropCharactorThread(void) {
    char vcText[2] = {
        0,
    };

    int iX = kRandom() % VGA_COLS;

    while (1) {
        kSleep(kRandom() % 20);

        if ((kRandom() % 20) < 16) {
            vcText[0] = ' ';
            for (int i = 0; i < VGA_ROWS - 1; i++) {
                kPrintAt(iX, i, vcText);
                kSleep(50);
            }
        } else {
            for (int i = 0; i < VGA_ROWS - 1; i++) {
                vcText[0] = i + kRandom();
                kPrintAt(iX, i, vcText);
                kSleep(50);
            }
        }
    }
}

static void kMatrixProcess(void) {
    for (int i = 0; i < 300; i++) {
        if (kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kDropCharactorThread) == NULL) {
            break;
        }

        kSleep(kRandom() % 5 + 5);
    }

    kGetCh();
}

static void kShowMatrix(PARAMETER_LIST* pstList) {
    kClear(0);

    TCB* pstProcess =
        kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess);
    if (pstProcess != NULL) {
        while ((pstProcess->stLink.qwID >> 32) != 0) {
            kSleep(100);
        }

        kClear(0);
    } else {
        kPrintlnColor("Matrix Process Create Fail", VGA_ATTR_FOREGROUND_BRIGHTRED);
    }
}

/**
 *  하드 디스크의 정보를 표시
 */
static void kShowHDDInformation(PARAMETER_LIST* pstList) {
    HDDINFORMATION stHDD;
    char vcBuffer[100];

    if (kReadHDDInformation(TRUE, TRUE, &stHDD) == FALSE) {
        kPrintErr("HDD Information Read Fail\n");
        return;
    }

    kPrintln("======== HDD Information (ATA, primary, master) ========");

    // 모델 번호 출력
    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
    kPrintf("Model Number:\t %s\n", vcBuffer);

    // 시리얼 번호 출력
    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
    kPrintf("Serial Number:\t %s\n", vcBuffer);

    // 헤드, 실린더, 실린더 당 섹터 수를 출력
    kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

    // 총 섹터 수 출력
    kPrintf("Total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);
}

static void kReadHDD(PARAMETER_LIST* pstList) {
    char vcLBA[50];
    if ((kGetNextParameter(pstList, vcLBA) == 0)) {
        kPrintln("[Usage] readhdd <LBA>");
        return;
    }

    DWORD dwLBA = kAToI(vcLBA, 10);
    char* pcBuffer = kAllocateMemory(512);

    if (kReadATASector(TRUE, TRUE, dwLBA, 1, pcBuffer) == 0) {
        kPrintln("Read Fail");
        kFreeMemory(pcBuffer);

        return;
    }

    kPrintln(pcBuffer);

    kFreeMemory(pcBuffer);
}

static void kWriteHDD(PARAMETER_LIST* pstList) {
    char vcLBA[50];
    char data[512] = {
        0,
    };
    if ((kGetNextParameter(pstList, vcLBA) == 0)) {
        kPrintln("[Usage] writehdd <LBA> <TEXT>");
        return;
    }

    char* dataPt = data;

    while (1) {
        int size = kGetNextParameter(pstList, dataPt);
        if (size == 0) {
            break;
        }

        dataPt += size;
        *dataPt = ' ';
        dataPt++;
    }

    *(--dataPt) = '\0';

    DWORD dwLBA = kAToI(vcLBA, 10);

    if (kWriteATASector(TRUE, TRUE, dwLBA, 1, data) == 0) {
        kPrintln("Write Fail");

        return;
    }
}

SHELL_COMMAND_ENTRY gs_vstCommandTable[] = {
    {"help", "Show all commands", kHelp},
    {"clear", "Clear screen", kCls},
    {"echo", "Echo arguments", kEcho},
    {"reboot", "Reboot system", kReboot_},
    {"memory", "Show memory info", kShowMemoryInfo},
    {"cpuspeed", "Measure processor speed", kMeasureProcessorSpeed},
    {"cpuload", "Show processor load", kCPULoad},
    {"wait", "Wait ms using PIT.", kWaitUsingPIT},
    {"datetime", "Show date and time", kShowDateAndTime},
    {"createtask", "Create task", kCreateTestTask},
    {"changepriority", "Change task priority", kChangeTaskPriority},
    {"tasklist", "Show task list", kShowTaskList},
    {"kill", "Kill task", kKillTask},
    {"matrix", "Show MATRIX", kShowMatrix},
    {"hddinfo", "Show HDD info", kShowHDDInformation},
    {"readhdd", "Read from HDD", kReadHDD},
    {"writehdd", "Write to HDD", kWriteHDD},
};

static void kHelp(PARAMETER_LIST* pstList) {
    int i;
    int iCount;
    int iCursorX, iCursorY;
    int iLength, iMaxCommandLength = 0;

    kPrintlnColor("     __  __   ____    _____", SHELL_COMMAND_ATTR);
    kPrintlnColor("    / / / /  / __ \\  / ___/", SHELL_COMMAND_ATTR);
    kPrintlnColor("   / /_/ /  / / / /  \\__ \\", SHELL_COMMAND_ATTR);
    kPrintlnColor("  / __  /  / /_/ /  ___/ /", SHELL_COMMAND_ATTR);
    kPrintlnColor(" /_/ /_/   \\____/  /____/   ", SHELL_COMMAND_ATTR);
    kPrintln("");

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELL_COMMAND_ENTRY);

    // 가장 긴 커맨드의 길이를 계산
    for (i = 0; i < iCount; i++) {
        iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
        if (iLength > iMaxCommandLength) {
            iMaxCommandLength = iLength;
        }
    }

    // 도움말 출력
    for (i = 0; i < iCount; i++) {
        kPrintColor(gs_vstCommandTable[i].pcCommand, SHELL_COMMAND_ATTR);

        int spaceLen = iMaxCommandLength - kStrLen(gs_vstCommandTable[i].pcCommand) + 1;
        for (int j = 0; j < spaceLen; j++) {
            kPrint(" ");
        }

        kPrintf(": %s\n", gs_vstCommandTable[i].pcDescription);
    }
}

////////////////////////////////////////////////////////////////
// Shell
////////////////////////////////////////////////////////////////
void kStartConsoleShell(void) {
    char vcCommandBuffer[SHELL_MAXCOMMANDBUFFERCOUNT];
    int iCommandBufferIndex = 0;
    int iCursorX, iCursorY;

    kPrintColor(SHELL_PROMPT_MESSAGE, SHELL_PROMPT_ATTR);

    while (1) {
        BYTE bKey = kGetCh();

        if (bKey == KEY_ENTER) {
            kPrint("\n");

            if (iCommandBufferIndex > 0) {
                vcCommandBuffer[iCommandBufferIndex] = '\0';
                kExecuteCommand(vcCommandBuffer);
            }

            kPrintColor(SHELL_PROMPT_MESSAGE, SHELL_PROMPT_ATTR);
            kMemSet(vcCommandBuffer, 0, SHELL_MAXCOMMANDBUFFERCOUNT);
            iCommandBufferIndex = 0;
        } else if (bKey == KEY_BACKSPACE) {
            if (iCommandBufferIndex <= 0) {
                continue;
            }

            kClearChar();
            iCommandBufferIndex--;
        } else if ((bKey == KEY_LSHIFT) || (bKey == KEY_RSHIFT) || (bKey == KEY_CAPSLOCK) || (bKey == KEY_NUMLOCK) ||
                   (bKey == KEY_SCROLLLOCK)) {
            // do nothing
        } else {
            if (iCommandBufferIndex < SHELL_MAXCOMMANDBUFFERCOUNT) {
                vcCommandBuffer[iCommandBufferIndex++] = bKey;
                kPrintf("%c", bKey);
            }
        }
    }
}

void kExecuteCommand(const char* pcCommandBuffer) {
    int i, iSpaceIndex;

    int iCommandBufferLength = kStrLen(pcCommandBuffer);
    for (iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++) {
        if (pcCommandBuffer[iSpaceIndex] == ' ') {
            break;
        }
    }

    int iCount = sizeof(gs_vstCommandTable) / sizeof(SHELL_COMMAND_ENTRY);
    for (i = 0; i < iCount; i++) {
        int iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

        if ((iCommandLength == iSpaceIndex) &&
            (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)) {

            PARAMETER_LIST stList;
            kInitializeParameter(&stList, pcCommandBuffer + iSpaceIndex + 1);

            gs_vstCommandTable[i].pfFunction(&stList);

            break;
        }
    }

    if (i >= iCount) {
        kPrintf("command not found: %s\n", pcCommandBuffer);
    }
}

void kInitializeParameter(PARAMETER_LIST* pstList, const char* pcParameter) {
    pstList->pcBuffer = pcParameter;
    pstList->iLength = kStrLen(pcParameter);
    pstList->iCurrentPosition = 0;
}

int kGetNextParameter(PARAMETER_LIST* pstList, char* pcParameter) {
    int i;
    int iLength;

    if (pstList->iLength <= pstList->iCurrentPosition) {
        return 0;
    }

    for (i = pstList->iCurrentPosition; i < pstList->iLength; i++) {
        if (pstList->pcBuffer[i] == ' ') {
            break;
        }
    }

    kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
    iLength = i - pstList->iCurrentPosition;
    pcParameter[iLength] = '\0';

    pstList->iCurrentPosition += iLength + 1;

    return iLength;
}