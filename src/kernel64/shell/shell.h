#ifndef __SHELL_H__
#define __SHELL_H__

#include "../io/video.h"
#include "../types.h"

#define SHELL_MAXCOMMANDBUFFERCOUNT 300
#define SHELL_PROMPT_MESSAGE "HOS > "

#define SHELL_PROMPT_ATTR (VGA_ATTR_BACKGROUND_BLACK | VGA_ATTR_FOREGROUND_DARKWHITE)
#define SHELL_COMMAND_ATTR (VGA_ATTR_BACKGROUND_BLACK | VGA_ATTR_FOREGROUND_DARKCYAN)

#pragma pack(push, 1)

typedef struct kParameterListStruct {
    const char* pcBuffer;
    int iLength;
    int iCurrentPosition;
} PARAMETER_LIST;

// 문자열 포인터를 파라미터로 받는 함수 포인터 타입 정의
typedef void (*CommandFunction)(PARAMETER_LIST* pstList);

typedef struct kShellCommandEntryStruct {
    char* pcCommand;
    char* pcDescription;
    CommandFunction pfFunction;
} SHELL_COMMAND_ENTRY;

#pragma pack(pop)

void kStartConsoleShell(void);
void kExecuteCommand(const char* pcCommandBuffer);
void kInitializeParameter(PARAMETER_LIST* pstList, const char* pcParameter);
int kGetNextParameter(PARAMETER_LIST* pstList, char* pcParameter);

#endif /*__SHELL_H__*/