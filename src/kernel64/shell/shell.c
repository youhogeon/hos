
#include "shell.h"
#include "../fs/mintfs.h"
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

static void kShowHDDInformation(PARAMETER_LIST* pstList) {
    HDDINFORMATION stHDD;
    char vcBuffer[100];

    if (kReadATAInformation(TRUE, TRUE, &stHDD) == FALSE) {
        kPrintErr("HDD Information Read Fail\n");
        return;
    }

    kPrintln("======== HDD Information ========");
    kPrintln("Driver:\t\t\t ATA, primary, master");

    kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
    vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
    kPrintf("Model Number:\t %s\n", vcBuffer);

    kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
    vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
    kPrintf("Serial Number:\t %s\n", vcBuffer);

    kPrintf("Head Count:\t\t %d\n", stHDD.wNumberOfHead);
    kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
    kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);
    kPrintf("Total Sector:\t %d\n", stHDD.dwTotalSectors);
    kPrintf("Total Size:\t\t %dMB\n", stHDD.dwTotalSectors / 2 / 1024);
}

static void kFormatHDD(PARAMETER_LIST* pstList) {
    if (kFormat() == FALSE) {
        kPrintErr("HDD format failed.\n");

        return;
    }

    kPrintln("HDD formatted.");
}

static void kShowRootDirectory(PARAMETER_LIST* pstList) {
    DIR* pstDirectory = opendir("/");
    if (pstDirectory == NULL) {
        kPrintErr("Could not open root directory.\n");
        return;
    }

    int iTotalCount = 0;
    DWORD dwTotalByte = 0;
    DWORD dwUsedClusterCount = 0;
    while (1) {
        struct dirent* pstEntry = readdir(pstDirectory);
        if (pstEntry == NULL) {
            // 더이상 파일이 없으면 나감
            break;
        }

        iTotalCount++;
        dwTotalByte += pstEntry->dwFileSize;

        // 실제로 사용된 클러스터의 개수를 계산
        if (pstEntry->dwFileSize == 0) {
            // 크기가 0이라도 클러스터 1개는 할당되어 있음
            dwUsedClusterCount++;
        } else {
            // 클러스터 개수를 올림하여 더함
            dwUsedClusterCount += (pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE;
        }
    }

    // 실제 파일의 내용을 표시하는 루프
    rewinddir(pstDirectory);
    int iCount = 0;
    while (1) {
        // 디렉터리에서 엔트리 하나를 읽음
        struct dirent* pstEntry = readdir(pstDirectory);
        // 더이상 파일이 없으면 나감
        if (pstEntry == NULL) {
            break;
        }

        // 전부 공백으로 초기화 한 후 각 위치에 값을 대입
        char vcBuffer[400];
        kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);

        // 파일 이름 삽입
        kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

        // 파일 길이 삽입
        char vcTempValue[50];
        kSPrintf(vcTempValue, "%d Byte", pstEntry->dwFileSize);
        kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

        // 파일의 시작 클러스터 삽입
        kSPrintf(vcTempValue, "0x%X Cluster", pstEntry->dwStartClusterIndex);
        kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
        kPrintf("    %s\n", vcBuffer);

        if ((iCount != 0) && ((iCount % 20) == 0)) {
            kPrintf("Press any key to continue... ('q' to quit) : ");
            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
        }

        iCount++;
    }

    // 총 파일의 개수와 파일의 총 크기를 출력
    kPrintf("\n");
    kPrintf("Total File Count: %d\n", iTotalCount);
    kPrintf("Total File Size: %d KByte (%d Cluster)\n", dwTotalByte, dwUsedClusterCount);

    // // 남은 클러스터 수를 이용해서 여유 공간을 출력
    FILESYSTEMMANAGER stManager;
    kGetFileSystemInformation(&stManager);
    kPrintf("Left Space: %d KByte (%d Cluster)\n",
            (stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024,
            stManager.dwTotalClusterCount - dwUsedClusterCount);

    closedir(pstDirectory);
}

static void kCreateFileInRootDirectory(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength = kGetNextParameter(pstList, vcFileName);

    if (iLength == 0) {
        kPrintln("[Usage] touch <file name>");
        return;
    }

    if (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) {
        kPrintErr("Invalid file name.\n");
        return;
    }

    FILE* pstFile = fopen(vcFileName, "w");
    if (pstFile == NULL) {
        kPrintErr("File create failed.\n");

        return;
    }

    fclose(pstFile);
    kPrintln("File created.");
}

static void kDeleteFileInRootDirectory(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength = kGetNextParameter(pstList, vcFileName);

    if (iLength == 0) {
        kPrintln("[Usage] rm <file name>");
        return;
    }

    if (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) {
        kPrintErr("Invalid file name.\n");
        return;
    }

    if (remove(vcFileName) != 0) {
        kPrintErr("Could not delete file.\n");
        return;
    }

    kPrintln("File deleted.");
}

static void kWriteDataToFile(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength = kGetNextParameter(pstList, vcFileName);

    if (iLength == 0) {
        kPrintln("[Usage] write <file name>");
        return;
    }

    if (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) {
        kPrintErr("Invalid file name.\n");
        return;
    }

    FILE* fp = fopen(vcFileName, "w");
    if (fp == NULL) {
        kPrintErr("Could not open file.\n");
        return;
    }

    kPrintColor("Enter data to write to file. (Press ESC key twice to end)\n", VGA_ATTR_FOREGROUND_BRIGHTBLACK);

    // esc 키가 연속으로 2번 눌러질 때까지 내용을 파일에 씀
    int iEnterCount = 0;
    int size = 0;
    while (1) {
        BYTE bKey = kGetCh();
        if (bKey == KEY_ESC) {
            iEnterCount++;
            if (iEnterCount >= 2) {
                break;
            }

            continue;
        }

        iEnterCount = 0;

        if (bKey == KEY_BACKSPACE) {
            if (size == 0) {
                continue;
            }

            size--;
            fseek(fp, -1, SEEK_CUR);
            kClearChar();

            continue;
        }

        kPrintf("%c", bKey);
        if (fwrite(&bKey, 1, 1, fp) != 1) {
            kPrintErr("Could not write file.\n");
            break;
        }

        size++;
    }

    kPrintf("\nFile Create Success\n");
    fclose(fp);
}

static void kReadDataFromFile(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength = kGetNextParameter(pstList, vcFileName);

    if (iLength == 0) {
        kPrintln("[Usage] cat <file name>");
        return;
    }

    if (iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)) {
        kPrintErr("Invalid file name.\n");
        return;
    }

    // 파일 생성
    FILE* fp = fopen(vcFileName, "r");
    if (fp == NULL) {
        kPrintErr("Could not open file.\n");
        return;
    }

    // 파일의 끝까지 출력하는 것을 반복
    int iEnterCount = 0;
    while (1) {
        BYTE bKey;
        if (fread(&bKey, 1, 1, fp) != 1) {
            break;
        }

        kPrintf("%c", bKey);

        if (bKey == KEY_ENTER) {
            iEnterCount++;

            if ((iEnterCount != 0) && ((iEnterCount % 20) == 0)) {
                kPrintf("Press any key to continue... ('q' to quit) : ");
                if (kGetCh() == 'q') {
                    kPrintf("\n");
                    break;
                }

                kPrintf("\n");
                iEnterCount = 0;
            }
        }
    }

    fclose(fp);
    kPrintf("\n");
}

SHELL_COMMAND_ENTRY gs_vstCommandTable[] = {
    {"help", "Show all commands", kHelp},
    {"clear", "Clear screen", kCls},
    {"reboot", "Reboot system", kReboot_},
    {"memory", "Show memory info", kShowMemoryInfo},
    {"cpuspeed", "Measure processor speed", kMeasureProcessorSpeed},
    {"cpuload", "Show processor load", kCPULoad},
    {"wait", "Wait ms using PIT.", kWaitUsingPIT},
    {"datetime", "Show date and time", kShowDateAndTime},
    {"changepriority", "Change task priority", kChangeTaskPriority},
    {"tasklist", "Show task list", kShowTaskList},
    {"kill", "Kill task", kKillTask},
    {"matrix", "Show MATRIX", kShowMatrix},
    {"hddinfo", "Show HDD info", kShowHDDInformation},
    {"formathdd", "Format HDD", kFormatHDD},
    {"ls", "Show files", kShowRootDirectory},
    {"touch", "Create file", kCreateFileInRootDirectory},
    {"write", "Write data to file", kWriteDataToFile},
    {"cat", "Read data from file", kReadDataFromFile},
    {"rm", "Delete file", kDeleteFileInRootDirectory},
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