#include "keyboard.h"
#include "../task/scheduler.h"
#include "../util/assembly.h"
#include "../util/queue.h"
#include "../util/sync.h"

static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

static BOOL kIsOutputBufferFull(void) { return (inb(0x64) & 0x01) != 0; }

static BOOL kIsInputBufferFull(void) { return (inb(0x64) & 0x02) != 0; }

static void waitInputBufferEmpty(void) {
    for (int i = 0; i < 0xFFFF; i++) {
        if (kIsInputBufferFull() == FALSE) {
            return;
        }
    }
}

static void waitOutputBufferFull(void) {
    for (int i = 0; i < 0xFFFF; i++) {
        if (kIsOutputBufferFull() == TRUE) {
            return;
        }
    }
}

static BOOL waitACK(void) {
    for (int i = 0; i < 0x80; i++) {
        // Wait until the output buffer is full
        waitOutputBufferFull();

        BYTE bData = inb(0x60);

        if (bData == 0xFA) {
            return TRUE;
        }

        kConvertScanCodeAndPutQueue(bData);
    }

    return FALSE;
}

BOOL kInitKeyboard(void) {
    kInitQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));

    if (!kActivateKeyboard()) {
        return FALSE;
    }

    kChangeKeyboardLED(FALSE, FALSE, FALSE);

    return TRUE;
}

BOOL kActivateKeyboard(void) {
    outb(0x64, 0xAE); // enable keyboard

    // Wait until the input buffer is empty
    waitInputBufferEmpty();

    outb(0x60, 0xF4); // activate keyboard

    // Wait for ACK(0xFA)
    if (waitACK() == FALSE) {
        return FALSE;
    }

    if (kChangeKeyboardLED(FALSE, FALSE, FALSE) == FALSE) {
        return FALSE;
    }

    return TRUE;
}

BOOL kGetKeyAndPutQueue(void) {
    if (kIsOutputBufferFull() == FALSE) {
        return FALSE;
    }

    BYTE scanCode = inb(0x60);
    return kConvertScanCodeAndPutQueue(scanCode);
}

BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn) {
    int i, j;

    // 출력 버퍼(포트 0x60)로 LED 상태 변경 커맨드(0xED) 전송
    waitInputBufferEmpty();
    outb(0x60, 0xED);
    waitInputBufferEmpty();

    if (waitACK() == FALSE) {
        return FALSE;
    }

    // LED 변경 값을 키보드로 전송
    outb(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | bScrollLockOn);
    waitInputBufferEmpty();

    if (waitACK() == FALSE) {
        return FALSE;
    }

    return TRUE;
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode) {
    KEYDATA stData;
    BOOL bResult = FALSE;

    stData.bScanCode = bScanCode;

    if (kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE) {
        BOOL bPreviousFlag = kLockForSystemData();
        bResult = kPutQueue(&gs_stKeyQueue, &stData);
        kUnlockForSystemData(bPreviousFlag);
    }

    return bResult;
}

BOOL kGetKeyFromKeyQueue(KEYDATA* pstData) {
    BOOL bResult;

    if (kIsQueueEmpty(&gs_stKeyQueue) == TRUE) {
        return FALSE;
    }

    BOOL bPreviousFlag = kLockForSystemData();
    bResult = kGetQueue(&gs_stKeyQueue, pstData);
    kUnlockForSystemData(bPreviousFlag);

    return bResult;
}

BYTE kGetCh(void) {
    KEYDATA stData;

    while (1) {
        while (kGetKeyFromKeyQueue(&stData) == FALSE) {
            kSchedule();
        }

        if (stData.bFlags & KEY_FLAGS_DOWN) {
            return stData.bASCIICode;
        }
    }
}

void kReboot(void) {
    waitInputBufferEmpty();

    outb(0x64, 0xD1);
    outb(0x60, 0x00);

    while (1) {
    }
}

////////////////////////////////////////////////////////////////
// Scan Code Conversion
////////////////////////////////////////////////////////////////
// 키보드 상태를 관리하는 키보드 매니저
static KEYBOARDMANAGER gs_stKeyboardManager = {
    0,
};

// 스캔 코드를 ASCII 코드로 변환하는 테이블
static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
    {KEY_NONE, KEY_NONE},             /*  0   */
    {KEY_ESC, KEY_ESC},               /*  1   */
    {'1', '!'},                       /*  2   */
    {'2', '@'},                       /*  3   */
    {'3', '#'},                       /*  4   */
    {'4', '$'},                       /*  5   */
    {'5', '%'},                       /*  6   */
    {'6', '^'},                       /*  7   */
    {'7', '&'},                       /*  8   */
    {'8', '*'},                       /*  9   */
    {'9', '('},                       /*  10  */
    {'0', ')'},                       /*  11  */
    {'-', '_'},                       /*  12  */
    {'=', '+'},                       /*  13  */
    {KEY_BACKSPACE, KEY_BACKSPACE},   /*  14  */
    {KEY_TAB, KEY_TAB},               /*  15  */
    {'q', 'Q'},                       /*  16  */
    {'w', 'W'},                       /*  17  */
    {'e', 'E'},                       /*  18  */
    {'r', 'R'},                       /*  19  */
    {'t', 'T'},                       /*  20  */
    {'y', 'Y'},                       /*  21  */
    {'u', 'U'},                       /*  22  */
    {'i', 'I'},                       /*  23  */
    {'o', 'O'},                       /*  24  */
    {'p', 'P'},                       /*  25  */
    {'[', '{'},                       /*  26  */
    {']', '}'},                       /*  27  */
    {'\n', '\n'},                     /*  28  */
    {KEY_CTRL, KEY_CTRL},             /*  29  */
    {'a', 'A'},                       /*  30  */
    {'s', 'S'},                       /*  31  */
    {'d', 'D'},                       /*  32  */
    {'f', 'F'},                       /*  33  */
    {'g', 'G'},                       /*  34  */
    {'h', 'H'},                       /*  35  */
    {'j', 'J'},                       /*  36  */
    {'k', 'K'},                       /*  37  */
    {'l', 'L'},                       /*  38  */
    {';', ':'},                       /*  39  */
    {'\'', '\"'},                     /*  40  */
    {'`', '~'},                       /*  41  */
    {KEY_LSHIFT, KEY_LSHIFT},         /*  42  */
    {'\\', '|'},                      /*  43  */
    {'z', 'Z'},                       /*  44  */
    {'x', 'X'},                       /*  45  */
    {'c', 'C'},                       /*  46  */
    {'v', 'V'},                       /*  47  */
    {'b', 'B'},                       /*  48  */
    {'n', 'N'},                       /*  49  */
    {'m', 'M'},                       /*  50  */
    {',', '<'},                       /*  51  */
    {'.', '>'},                       /*  52  */
    {'/', '?'},                       /*  53  */
    {KEY_RSHIFT, KEY_RSHIFT},         /*  54  */
    {'*', '*'},                       /*  55  */
    {KEY_LALT, KEY_LALT},             /*  56  */
    {' ', ' '},                       /*  57  */
    {KEY_CAPSLOCK, KEY_CAPSLOCK},     /*  58  */
    {KEY_F1, KEY_F1},                 /*  59  */
    {KEY_F2, KEY_F2},                 /*  60  */
    {KEY_F3, KEY_F3},                 /*  61  */
    {KEY_F4, KEY_F4},                 /*  62  */
    {KEY_F5, KEY_F5},                 /*  63  */
    {KEY_F6, KEY_F6},                 /*  64  */
    {KEY_F7, KEY_F7},                 /*  65  */
    {KEY_F8, KEY_F8},                 /*  66  */
    {KEY_F9, KEY_F9},                 /*  67  */
    {KEY_F10, KEY_F10},               /*  68  */
    {KEY_NUMLOCK, KEY_NUMLOCK},       /*  69  */
    {KEY_SCROLLLOCK, KEY_SCROLLLOCK}, /*  70  */
    {KEY_HOME, '7'},                  /*  71  */
    {KEY_UP, '8'},                    /*  72  */
    {KEY_PAGEUP, '9'},                /*  73  */
    {'-', '-'},                       /*  74  */
    {KEY_LEFT, '4'},                  /*  75  */
    {KEY_CENTER, '5'},                /*  76  */
    {KEY_RIGHT, '6'},                 /*  77  */
    {'+', '+'},                       /*  78  */
    {KEY_END, '1'},                   /*  79  */
    {KEY_DOWN, '2'},                  /*  80  */
    {KEY_PAGEDOWN, '3'},              /*  81  */
    {KEY_INS, '0'},                   /*  82  */
    {KEY_DEL, '.'},                   /*  83  */
    {KEY_NONE, KEY_NONE},             /*  84  */
    {KEY_NONE, KEY_NONE},             /*  85  */
    {KEY_NONE, KEY_NONE},             /*  86  */
    {KEY_F11, KEY_F11},               /*  87  */
    {KEY_F12, KEY_F12},               /*  88  */
};

BOOL kIsAlphabetScanCode(BYTE bScanCode) {
    BYTE code = gs_vstKeyMappingTable[bScanCode].bNormalCode;

    if ('a' <= code && code <= 'z') {
        return TRUE;
    }

    return FALSE;
}

BOOL kIsNumberOrSymbolScanCode(BYTE bScanCode) {
    if (2 <= bScanCode && bScanCode <= 53 && kIsAlphabetScanCode(bScanCode) == FALSE) {
        return TRUE;
    }

    return FALSE;
}

BOOL kIsNumberPadScanCode(BYTE bScanCode) {
    if (71 <= bScanCode && bScanCode <= 83) {
        return TRUE;
    }

    return FALSE;
}

BOOL kIsUseCombinedCode(BYTE bScanCode) {
    BYTE bDownScanCode;
    BOOL bUseCombinedKey;

    bDownScanCode = bScanCode & 0x7F;

    if (kIsAlphabetScanCode(bDownScanCode) == TRUE) {
        if (gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn) {
            bUseCombinedKey = TRUE;
        } else {
            bUseCombinedKey = FALSE;
        }
    } else if (kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE) {
        if (gs_stKeyboardManager.bShiftDown == TRUE) {
            bUseCombinedKey = TRUE;
        } else {
            bUseCombinedKey = FALSE;
        }
    } else if (kIsNumberPadScanCode(bDownScanCode) == TRUE && gs_stKeyboardManager.bExtendedCodeIn == FALSE) {
        if (gs_stKeyboardManager.bNumLockOn == TRUE) {
            bUseCombinedKey = TRUE;
        } else {
            bUseCombinedKey = FALSE;
        }
    }

    return bUseCombinedKey;
}

static void _updateCombinationKeyStatusAndLED(BYTE bScanCode) {
    BOOL bDown;
    BYTE bDownScanCode;
    BOOL bLEDStatusChanged = FALSE;

    if (bScanCode & 0x80) {
        bDown = FALSE;
        bDownScanCode = bScanCode & 0x7F;
    } else {
        bDown = TRUE;
        bDownScanCode = bScanCode;
    }

    // 조합 키 검색
    if (bDownScanCode == 42 || bDownScanCode == 54) {
        // Shift 키의 스캔 코드(42 or 54)이면 Shift 키의 상태 갱신
        gs_stKeyboardManager.bShiftDown = bDown;
    } else if (bDownScanCode == 58 && bDown == TRUE) {
        // Caps Lock 키의 스캔 코드(58)이면 Caps Lock의 상태 갱신하고 LED 상태
        // 변경
        gs_stKeyboardManager.bCapsLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    } else if (bDownScanCode == 69 && bDown == TRUE) {
        // Num Lock 키의 스캔 코드(69)이면 Num Lock의 상태를 갱신하고 LED 상태
        // 변경
        gs_stKeyboardManager.bNumLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    } else if (bDownScanCode == 70 && bDown == TRUE) {
        // Scroll Lock 키의 스캔 코드(70)이면 Scroll Lock의 상태를 갱신하고 LED
        // 상태 변경
        gs_stKeyboardManager.bScrollLockOn ^= TRUE;
        bLEDStatusChanged = TRUE;
    }

    if (bLEDStatusChanged == TRUE) {
        kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn,
                           gs_stKeyboardManager.bNumLockOn,
                           gs_stKeyboardManager.bScrollLockOn);
    }
}

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BOOL* pbFlags) {
    // 이전에 Pause 키가 수신되었다면, Pause의 남은 스캔 코드를 무시
    if (gs_stKeyboardManager.iSkipCountForPause > 0) {
        gs_stKeyboardManager.iSkipCountForPause--;
        return FALSE;
    }

    if (bScanCode == 0xE1) { // pause key
        *pbASCIICode = KEY_PAUSE;
        *pbFlags = KEY_FLAGS_DOWN;
        gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;

        return TRUE;
    } else if (bScanCode == 0xE0) { // extended key
        gs_stKeyboardManager.bExtendedCodeIn = TRUE;

        return FALSE;
    }

    BOOL bUseCombinedKey = kIsUseCombinedCode(bScanCode);

    if (bUseCombinedKey == TRUE) {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;
    } else {
        *pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;
    }

    if (gs_stKeyboardManager.bExtendedCodeIn == TRUE) {
        *pbFlags = KEY_FLAGS_EXTENDEDKEY;
        gs_stKeyboardManager.bExtendedCodeIn = FALSE;
    } else {
        *pbFlags = 0;
    }

    if ((bScanCode & 0x80) == 0) {
        *pbFlags |= KEY_FLAGS_DOWN;
    }

    _updateCombinationKeyStatusAndLED(bScanCode);

    return TRUE;
}