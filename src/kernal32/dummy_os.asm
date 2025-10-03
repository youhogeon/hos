[ORG 0x0]
[BITS 16]

SECTION .text

jmp 0x1000:BEGIN

SECTOR_IDX: dw 0
TOTAL_SECTOR_COUNT: equ 1024

BEGIN:
    mov ax, cs
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    %assign i 0
    %rep TOTAL_SECTOR_COUNT
        %assign i i + 1

        mov ax, word[ SECTOR_IDX ]
        shl ax, 1
        mov si, ax
        mov byte[ es: si + (160 * 2) ], '0' + (i % 10)

        inc word[ SECTOR_IDX ]

        %if i == TOTAL_SECTOR_COUNT
            jmp $
        %else
            jmp ( 0x1000 + i * 0x20): 0x0000
        %endif

        times (512 - ($ - $$) % 512) db 0x00
    %endrep