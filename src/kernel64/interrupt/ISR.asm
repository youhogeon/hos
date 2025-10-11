[BITS 64]

%include "./util/macro.inc"

SECTION .text

extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler

; Exception ISR
global kISR_DivideError, kISR_Debug, kISR_NMI, kISR_BreakPoint, kISR_Overflow
global kISR_BoundRangeExceeded, kISR_InvalidOpcode, kISR_DeviceNotAvailable, kISR_DoubleFault,
global kISR_CoprocessorSegmentOverrun, kISR_InvalidTSS, kISR_SegmentNotPresent
global kISR_StackSegmentFault, kISR_GeneralProtection, kISR_PageFault, kISR_15
global kISR_FPUError, kISR_AlignmentCheck, kISR_MachineCheck, kISR_SIMDError, kISR_ETCException

; Interrupt ISR
global kISR_Timer, kISR_Keyboard, kISR_SlavePIC, kISR_Serial2, kISR_Serial1, kISR_Parallel2
global kISR_Floppy, kISR_Parallel1, kISR_RTC, kISR_Reserved, kISR_NotUsed1, kISR_NotUsed2
global kISR_Mouse, kISR_Coprocessor, kISR_HDD1, kISR_HDD2, kISR_ETCInterrupt


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Exception Handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; #0, Divide Error ISR
kISR_DivideError:
    K_SAVE_CONTEXT

    mov rdi, 0
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #1, Debug ISR
kISR_Debug:
    K_SAVE_CONTEXT

    mov rdi, 1
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #2, NMI ISR
kISR_NMI:
    K_SAVE_CONTEXT

    mov rdi, 2
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #3, BreakPoint ISR
kISR_BreakPoint:
    K_SAVE_CONTEXT

    mov rdi, 3
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #4, Overflow ISR
kISR_Overflow:
    K_SAVE_CONTEXT

    mov rdi, 4
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #5, Bound Range Exceeded ISR
kISR_BoundRangeExceeded:
    K_SAVE_CONTEXT

    mov rdi, 5
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #6, Invalid Opcode ISR
kISR_InvalidOpcode:
    K_SAVE_CONTEXT

    mov rdi, 6
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #7, Device Not Available ISR
kISR_DeviceNotAvailable:
    K_SAVE_CONTEXT

    mov rdi, 7
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #8, Double Fault ISR
kISR_DoubleFault:
    K_SAVE_CONTEXT

    mov rdi, 8
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #9, Coprocessor Segment Overrun ISR
kISR_CoprocessorSegmentOverrun:
    K_SAVE_CONTEXT

    mov rdi, 9
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #10, Invalid TSS ISR
kISR_InvalidTSS:
    K_SAVE_CONTEXT

    mov rdi, 10
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #11, Segment Not Present ISR
kISR_SegmentNotPresent:
    K_SAVE_CONTEXT

    mov rdi, 11
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #12, Stack Segment Fault ISR
kISR_StackSegmentFault:
    K_SAVE_CONTEXT

    mov rdi, 12
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #13, General Protection ISR
kISR_GeneralProtection:
    K_SAVE_CONTEXT

    mov rdi, 13
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #14, Page Fault ISR
kISR_PageFault:
    K_SAVE_CONTEXT

    mov rdi, 14
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #15, Reserved ISR
kISR_15:
    K_SAVE_CONTEXT

    mov rdi, 15
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #16, FPU Error ISR
kISR_FPUError:
    K_SAVE_CONTEXT

    mov rdi, 16
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #17, Alignment Check ISR
kISR_AlignmentCheck:
    K_SAVE_CONTEXT

    mov rdi, 17
    mov rsi, qword [ rbp + 8 ]
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    add rsp, 8
    iretq

; #18, Machine Check ISR
kISR_MachineCheck:
    K_SAVE_CONTEXT

    mov rdi, 18
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #19, SIMD Floating Point Exception ISR
kISR_SIMDError:
    K_SAVE_CONTEXT

    mov rdi, 19
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq

; #20~#31, Reserved ISR
kISR_ETCException:
    K_SAVE_CONTEXT

    mov rdi, 20
    call kCommonExceptionHandler

    K_RESTORE_CONTEXT
    iretq


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Interrupt Handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; #32, 타이머 ISR
kISR_Timer:
    K_SAVE_CONTEXT

    mov rdi, 32
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #33, 키보드 ISR
kISR_Keyboard:
    K_SAVE_CONTEXT

    mov rdi, 33
    call kKeyboardHandler

    K_RESTORE_CONTEXT
    iretq

; #34, 슬레이브 PIC ISR
kISR_SlavePIC:
    K_SAVE_CONTEXT

    mov rdi, 34
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #35, 시리얼 포트 2 ISR
kISR_Serial2:
    K_SAVE_CONTEXT

    mov rdi, 35
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #36, 시리얼 포트 1 ISR
kISR_Serial1:
    K_SAVE_CONTEXT

    mov rdi, 36
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #37, 패러렐 포트 2 ISR
kISR_Parallel2:
    K_SAVE_CONTEXT

    mov rdi, 37
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #38, 플로피 디스크 컨트롤러 ISR
kISR_Floppy:
    K_SAVE_CONTEXT

    mov rdi, 38
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #39, 패러렐 포트 1 ISR
kISR_Parallel1:
    K_SAVE_CONTEXT

    mov rdi, 39
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #40, RTC ISR
kISR_RTC:
    K_SAVE_CONTEXT

    mov rdi, 40
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #41, 예약된 인터럽트의 ISR
kISR_Reserved:
    K_SAVE_CONTEXT

    mov rdi, 41
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #42, 사용 안함
kISR_NotUsed1:
    K_SAVE_CONTEXT

    mov rdi, 42
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #43, 사용 안함
kISR_NotUsed2:
    K_SAVE_CONTEXT

    mov rdi, 43
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #44, 마우스 ISR
kISR_Mouse:
    K_SAVE_CONTEXT

    mov rdi, 44
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #45, 코프로세서 ISR
kISR_Coprocessor:
    K_SAVE_CONTEXT

    mov rdi, 45
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #46, 하드 디스크 1 ISR
kISR_HDD1:
    K_SAVE_CONTEXT

    mov rdi, 46
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #47, 하드 디스크 2 ISR
kISR_HDD2:
    K_SAVE_CONTEXT

    mov rdi, 47
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq

; #48 이외의 모든 인터럽트에 대한 ISR
kISR_ETCInterrupt:
    K_SAVE_CONTEXT

    mov rdi, 48
    call kCommonInterruptHandler

    K_RESTORE_CONTEXT
    iretq