[BITS 64]

extern _start

SECTION .text

BEGIN:
    cli

    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    call _start
    jmp $
