
#include "shell.h"
#include "../io/PIT.h"
#include "../io/RTC.h"
#include "../io/keyboard.h"
#include "../io/video.h"
#include "../util/assembly.h"
#include "../util/memory.h"
#include "../util/string.h"

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

void kShowTotalRAMSize(PARAMETER_LIST* pstList) { kPrintf("Total RAM size: %d MB\n", kMemSize()); }

void kWaitUsingPIT(PARAMETER_LIST* pstList) {
    char vcParameter[100];
    int iLength;
    ;
    int i;

    // 파라미터 초기화
    if (kGetNextParameter(pstList, vcParameter) == 0) {
        kPrintln("[Usage] wait <ms>");
        return;
    }

    long lMillisecond = kAToI(vcParameter, 10);
    kPrintf("%d ms sleep...\n", lMillisecond);

    cli();
    for (i = 0; i < lMillisecond / 25; i++) {
        kWaitUsingDirectPIT(MSTOCOUNT(25));
    }
    kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 25));
    sti();

    kPrintf("%d ms sleep complete\n", lMillisecond);
    kInitPIT(MSTOCOUNT(1), TRUE);
}

void kMeasureProcessorSpeed(PARAMETER_LIST* pstList) {
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

void kShowDateAndTime(PARAMETER_LIST* pstList) {
    BYTE bSecond, bMinute, bHour;
    BYTE bDayOfWeek, bDayOfMonth, bMonth;
    WORD wYear;

    // RTC 컨트롤러에서 시간 및 일자를 읽음
    kReadRTCTime(&bHour, &bMinute, &bSecond);
    kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

    kPrintf("%d/%d/%d (%s) ", wYear, bMonth, bDayOfMonth, kConvertDayOfWeekToString(bDayOfWeek));
    kPrintf("%d:%d:%d\n", bHour, bMinute, bSecond);
}

SHELL_COMMAND_ENTRY gs_vstCommandTable[] = {
    {"help", "Show all commands", kHelp},
    {"clear", "Clear screen", kCls},
    {"echo", "Echo arguments", kEcho},
    {"reboot", "Reboot system", kReboot},
    {"memory", "Show total RAM size", kShowTotalRAMSize},
    {"cpuspeed", "Measure processor speed", kMeasureProcessorSpeed},
    {"wait", "Wait ms using PIT. [Usage] wait <ms>", kWaitUsingPIT},
    {"datetime", "Show date and time", kShowDateAndTime},
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