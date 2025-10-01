[ORG 0x7C00]
[BITS 16]

SECTION .text

xor ax, ax
mov ds, ax

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Load OS into 0x10000
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

READDATA:
    ; set destination
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov di, word[ TOTAL_SECTOR_COUNT ]

    .READLOOP:
        cmp di, 0
        je .READLOOP_END
        dec di

        ; call BIOS
        mov ah, 2
        mov al, 1
        mov ch, byte[ TRACK_IDX ]
        mov cl, byte[ SECTOR_IDX ]
        mov dh, byte[ HEAD_IDX ]
        ; mov dl, dl ; use initialized value by BIOS
        int 0x13
        jc HANDLE_DISK_ERROR

        ; increase
        add ax, 0x0020
        mov es, ax

        mov al, byte[ SECTOR_IDX ]
        inc al
        mov byte[ SECTOR_IDX ], al
        cmp al, 19
        jl .READLOOP

        xor byte[ HEAD_IDX ], 1
        mov byte[ SECTOR_IDX ], 1
        cmp byte[ HEAD_IDX ], 0
        jne .READLOOP

        add byte[ TRACK_IDX ], 1
        jmp .READLOOP


    .READLOOP_END:


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; HANDLER
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SUCCESS:
    mov ax, 0xB800
    mov es, ax

    xor di, di

    .CLEARLOOP:
        mov byte[ es: di ], 0
        mov byte[ es: di + 1 ], 0x0A

        add di, 2

        cmp di, 80 * 25 * 2
        jl .CLEARLOOP

        mov si, MESSAGE1
        xor di, di

    .MESSAGELOOP:
        mov al, byte[ si ]

        cmp al, 0
        je  .MESSAGELOOP_END

        mov byte[ es: di ], al

        inc si
        add di, 2

        jmp .MESSAGELOOP

    .MESSAGELOOP_END:
        jmp END

HANDLE_DISK_ERROR:
    mov ax, 0xB800
    mov es, ax

    xor di, di

    .CLEARLOOP:
        mov byte[ es: di ], 0
        mov byte[ es: di + 1 ], 0x0A

        add di, 2

        cmp di, 80 * 25 * 2
        jl .CLEARLOOP

        mov si, MESSAGE2
        xor di, di

    .MESSAGELOOP:
        mov al, byte[ si ]

        cmp al, 0
        je  .MESSAGELOOP_END

        mov byte[ es: di ], al

        inc si
        add di, 2

        jmp .MESSAGELOOP

    .MESSAGELOOP_END:
        jmp END

END:
    jmp $

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MESSAGE1: db 'Hello, World!', 0
MESSAGE2: db 'disk error!', 0

TOTAL_SECTOR_COUNT: dw 1024
SECTOR_IDX: db 2
HEAD_IDX: db 0
TRACK_IDX: db 0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Bootloader Signature
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

times 510 - ($ - $$) db 0x00
dw 0xAA55