
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

// static void kTestTask1(void) {
//     BYTE bData;
//     int i = 0, iX = 0, iY = 0, iMargin;
//     TCB* pstRunningTask;

//     // 자신의 ID를 얻어서 화면 오프셋으로 사용
//     pstRunningTask = kGetRunningTask();
//     iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

//     while (1) {
//         switch (i) {
//         case 0:
//             iX++;
//             if (iX >= (VGA_COLS - iMargin)) {
//                 i = 1;
//             }
//             break;

//         case 1:
//             iY++;
//             if (iY >= (VGA_ROWS - iMargin)) {
//                 i = 2;
//             }
//             break;

//         case 2:
//             iX--;
//             if (iX < iMargin) {
//                 i = 3;
//             }
//             break;

//         case 3:
//             iY--;
//             if (iY < iMargin) {
//                 i = 0;
//             }
//             break;
//         }

//         char d[2] = {bData, 0};
//         kPrintAt(iX, iY, d);
//         bData++;

//         kSchedule();
//     }
// }

// static void kTestTask2(void) {
//     char vcData[4] = {'-', '\\', '|', '/'};
//     int i = 0;

//     TCB* pstRunningTask = kGetRunningTask();
//     int id = (pstRunningTask->stLink.qwID & 0xFFFFFFFF);

//     while (1) {
//         char d[2] = {vcData[i % 4], 0};
//         kPrintAt(id % VGA_COLS, (id / VGA_COLS) % VGA_ROWS, d);

//         i++;

//         kSchedule();
//     }
// }

// static void kCreateTestTask(PARAMETER_LIST* pstList) {
//     char vcType[30];
//     char vcCount[30];

//     kGetNextParameter(pstList, vcType);
//     kGetNextParameter(pstList, vcCount);

//     int count = kAToI(vcCount, 10);
//     if (count <= 0) {
//         kPrintln("[Usage] createtask <type:1,2> <count>");
//         return;
//     }

//     switch (kAToI(vcType, 10)) {
//     case 1:
//         for (int i = 0; i < count; i++) {
//             if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL) {
//                 break;
//             }
//         }

//         kPrintf("Task1 Created\n");
//         break;

//     case 2:
//         for (int i = 0; i < count; i++) {
//             if (kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL) {
//                 break;
//             }
//         }

//         kPrintf("Task2 Created\n");
//         break;
//     default:
//         kPrintln("[Usage] createtask <type:1,2> <count>");
//     }
// }

// static void kChangeTaskPriority(PARAMETER_LIST* pstList) {
//     char vcID[30];
//     char vcPriority[30];
//     QWORD qwID;

//     kGetNextParameter(pstList, vcID);
//     kGetNextParameter(pstList, vcPriority);

//     if (kMemCmp(vcID, "0x", 2) == 0) {
//         qwID = kAToI(vcID + 2, 16);
//     } else {
//         qwID = kAToI(vcID, 10);
//     }

//     if (qwID == 0) {
//         kPrintln("[Usage] changepriority <ID> <PRIORITY:0,1,2,3,4)>");
//         return;
//     }

//     BYTE bPriority = kAToI(vcPriority, 10);

//     kPrintf("Change priority of task [0x%q] had changed to [%d]: ", qwID, bPriority);

//     if (kChangePriority(qwID, bPriority) == TRUE) {
//         kPrintlnColor("Success", VGA_ATTR_FOREGROUND_BRIGHTGREEN);
//     } else {
//         kPrintlnColor("Fail", VGA_ATTR_FOREGROUND_BRIGHTRED);
//     }
// }

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
/**
 *  하드 디스크를 연결
 */
static void kMountHDD(PARAMETER_LIST* pstList) {
    if (kMount() == FALSE) {
        kPrintf("HDD Mount Fail\n");
        return;
    }
    kPrintf("HDD Mount Success\n");
}

/**
 *  하드 디스크에 파일 시스템을 생성(포맷)
 */
static void kFormatHDD(PARAMETER_LIST* pstList) {
    if (kFormat() == FALSE) {
        kPrintf("HDD Format Fail\n");
        return;
    }
    kPrintf("HDD Format Success\n");
}

/**
 *  파일 시스템 정보를 표시
 */
static void kShowFileSystemInformation(PARAMETER_LIST* pstList) {
    FILESYSTEMMANAGER stManager;

    kGetFileSystemInformation(&stManager);

    kPrintf("================== File System Information ==================\n");
    kPrintf("Mouted:\t\t\t\t\t %d\n", stManager.bMounted);
    kPrintf("Reserved Sector Count:\t\t\t %d Sector\n", stManager.dwReservedSectorCount);
    kPrintf("Cluster Link Table Start Address:\t %d Sector\n", stManager.dwClusterLinkAreaStartAddress);
    kPrintf("Cluster Link Table Size:\t\t %d Sector\n", stManager.dwClusterLinkAreaSize);
    kPrintf("Data Area Start Address:\t\t %d Sector\n", stManager.dwDataAreaStartAddress);
    kPrintf("Total Cluster Count:\t\t\t %d Cluster\n", stManager.dwTotalClusterCount);
}

/**
 *  루트 디렉터리에 빈 파일을 생성
 */
static void kCreateFileInRootDirectory(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength;
    DWORD dwCluster;
    DIRECTORYENTRY stEntry;
    int i;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    iLength = kGetNextParameter(pstList, vcFileName);
    vcFileName[iLength] = '\0';
    if ((iLength > (sizeof(stEntry.vcFileName) - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    // 빈 클러스터를 찾아서 할당된 것으로 설정
    dwCluster = kFindFreeCluster();
    if ((dwCluster == FILESYSTEM_LASTCLUSTER) || (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE)) {
        kPrintf("Cluster Allocation Fail\n");
        return;
    }

    // 빈 디렉터리 엔트리를 검색
    i = kFindFreeDirectoryEntry();
    if (i == -1) {
        // 실패할 경우 할당 받은 클러스터를 반환해야 함
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        kPrintf("Directory Entry is Full\n");
        return;
    }

    // 디렉터리 엔트리를 설정
    kMemCpy(stEntry.vcFileName, vcFileName, iLength + 1);
    stEntry.dwStartClusterIndex = dwCluster;
    stEntry.dwFileSize = 0;

    // 디렉터리 엔트리를 등록
    if (kSetDirectoryEntryData(i, &stEntry) == FALSE) {
        // 실패할 경우 할당 받은 클러스터를 반환해야 함
        kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
        kPrintf("Directory Entry Set Fail\n");
    }
    kPrintf("File Create Success\n");
}

/**
 *  루트 디렉터리에서 파일을 삭제
 */
static void kDeleteFileInRootDirectory(PARAMETER_LIST* pstList) {
    char vcFileName[50];
    int iLength;
    DIRECTORYENTRY stEntry;
    int iOffset;

    // 파라미터 리스트를 초기화하여 파일 이름을 추출
    iLength = kGetNextParameter(pstList, vcFileName);
    vcFileName[iLength] = '\0';
    if ((iLength > (sizeof(stEntry.vcFileName) - 1)) || (iLength == 0)) {
        kPrintf("Too Long or Too Short File Name\n");
        return;
    }

    // 파일 이름으로 디렉터리 엔트리를 검색
    iOffset = kFindDirectoryEntry(vcFileName, &stEntry);
    if (iOffset == -1) {
        kPrintf("File Not Found\n");
        return;
    }

    // 클러스터를 반환
    if (kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE) {
        kPrintf("Cluster Free Fail\n");
        return;
    }

    // 디렉터리 엔트리를 모두 초기화하여 빈 것으로 설정한 뒤, 해당 오프셋에 덮어씀
    kMemSet(&stEntry, 0, sizeof(stEntry));
    if (kSetDirectoryEntryData(iOffset, &stEntry) == FALSE) {
        kPrintf("Root Directory Update Fail\n");
        return;
    }

    kPrintf("File Delete Success\n");
}

/**
 *  루트 디렉터리의 파일 목록을 표시
 */
static void kShowRootDirectory(PARAMETER_LIST* pstList) {
    BYTE* pbClusterBuffer;
    int i, iCount, iTotalCount;
    DIRECTORYENTRY* pstEntry;
    char vcBuffer[400];
    char vcTempValue[50];
    DWORD dwTotalByte;

    pbClusterBuffer = kAllocateMemory(FILESYSTEM_SECTORSPERCLUSTER * 512);

    // 루트 디렉터리를 읽음
    if (kReadCluster(0, pbClusterBuffer) == FALSE) {
        kPrintf("Root Directory Read Fail\n");
        return;
    }

    // 먼저 루프를 돌면서 디렉터리에 있는 파일의 개수와 전체 파일이 사용한 크기를 계산
    pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
    iTotalCount = 0;
    dwTotalByte = 0;
    for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (pstEntry[i].dwStartClusterIndex == 0) {
            continue;
        }
        iTotalCount++;
        dwTotalByte += pstEntry[i].dwFileSize;
    }

    // 실제 파일의 내용을 표시하는 루프
    pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
    iCount = 0;
    for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (pstEntry[i].dwStartClusterIndex == 0) {
            continue;
        }
        // 전부 공백으로 초기화 한 후 각 위치에 값을 대입
        kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
        vcBuffer[sizeof(vcBuffer) - 1] = '\0';

        // 파일 이름 삽입
        kMemCpy(vcBuffer, pstEntry[i].vcFileName, kStrLen(pstEntry[i].vcFileName));
        // 파일 길이 삽입
        kSPrintf(vcTempValue, "%d Byte", pstEntry[i].dwFileSize);
        kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));
        // 파일의 시작 클러스터 삽입
        kSPrintf(vcTempValue, "0x%X Cluster", pstEntry[i].dwStartClusterIndex);
        kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);
        kPrintf("    %s\n", vcBuffer);

        if ((iCount != 0) && ((iCount % 20) == 0)) {
            kPrintf("Press any key to continue... ('q' is exit) : ");
            if (kGetCh() == 'q') {
                kPrintf("\n");
                break;
            }
        }
        iCount++;
    }

    // 총 파일의 개수와 파일의 총 크기를 출력
    kPrintf("\t Total File Count: %d\t Total File Size: %d Byte\n", iTotalCount, dwTotalByte);

    kFreeMemory(pbClusterBuffer);
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
    // {"createtask", "Create task", kCreateTestTask},
    // {"changepriority", "Change task priority", kChangeTaskPriority},
    {"tasklist", "Show task list", kShowTaskList},
    {"kill", "Kill task", kKillTask},
    {"matrix", "Show MATRIX", kShowMatrix},
    {"hddinfo", "Show HDD info", kShowHDDInformation},
    {"readhdd", "Read from HDD", kReadHDD},
    {"writehdd", "Write to HDD", kWriteHDD},
    {"mounthdd", "Mount HDD", kMountHDD},
    {"formathdd", "Format HDD", kFormatHDD},
    {"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
    {"createfile", "Create File, ex)createfile a.txt", kCreateFileInRootDirectory},
    {"deletefile", "Delete File, ex)deletefile a.txt", kDeleteFileInRootDirectory},
    {"dir", "Show Directory", kShowRootDirectory},
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