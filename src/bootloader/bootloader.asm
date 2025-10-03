[ORG 0x7C00]
[BITS 16]

SECTION .text

cli
xor ax, ax
mov ds, ax
mov ss, ax
mov [BOOT_DRIVE], dl
mov sp, 0x7C00
sti

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Clear Display
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
mov ax, 0xB800
mov es, ax

xor di, di

.CLEARLOOP:
    mov byte[ es: di ], 0
    mov byte[ es: di + 1 ], 0x0A

    add di, 2

    cmp di, 80 * 25 * 2
    jl .CLEARLOOP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Load OS into 0x10000
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RESETDISK:
    mov dl, [BOOT_DRIVE]
    xor ax, ax
    int 0x13
    jc HANDLE_DISK_ERROR

READDATA:
    ; print
    push MESSAGE_READ_START
    call PRINT_MESSAGE
    add sp, 2

    mov si, 0x1000
    mov di, word[ TOTAL_SECTOR_COUNT ]

    .READLOOP:
        cmp di, 0
        je .READLOOP_END
        dec di

        ; set destination
        mov es, si
        xor bx, bx

        ; call BIOS
        mov ah, 2
        mov al, 1
        mov ch, byte[ TRACK_IDX ]
        mov cl, byte[ SECTOR_IDX ]
        mov dh, byte[ HEAD_IDX ]
        mov dl, byte[ BOOT_DRIVE ]
        int 0x13
        jc HANDLE_DISK_ERROR

        ; increase
        add si, 0x0020
        mov es, si

        inc byte[ SECTOR_IDX ]
        mov al, byte[ SECTOR_IDX ]
        cmp al, 37
        jl .READLOOP

        xor byte[ HEAD_IDX ], 1
        mov byte[ SECTOR_IDX ], 1
        cmp byte[ HEAD_IDX ], 0
        jne .READLOOP

        inc byte[ TRACK_IDX ]
        jmp .READLOOP

    .READLOOP_END:
        jmp HANDLE_SUCCESS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; HANDLER
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

HANDLE_SUCCESS:
    push MESSAGE_READ_END
    call PRINT_MESSAGE
    add sp, 2
    jmp HANDLE_END

HANDLE_DISK_ERROR:
    push MESSAGE_DISK_ERROR
    call PRINT_MESSAGE
    add sp, 2
    .INF_LOOP:
        cli
        hlt
        jmp .INF_LOOP

HANDLE_END:
    jmp 0x1000:0x0000


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; FUNCTIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PRINT_MESSAGE:
    push bp
    mov bp, sp
    push ax
    push bx
    push dx
    push es
    push di
    push si

    mov ax, 0xB800
    mov es, ax

    xor di, di
    mov si, word[bp + 4]

    .MESSAGELOOP:
        mov al, byte[ si ]

        cmp al, 0
        je  .MESSAGELOOP_END

        mov byte[ es: di ], al

        inc si
        add di, 2

        jmp .MESSAGELOOP

    .MESSAGELOOP_END:
        mov bx, di
        shr bx, 1
        mov ah, 2
        mov bh, 0
        mov dh, 0
        mov dl, bl
        int 0x10

    pop si
    pop di
    pop es
    pop dx
    pop bx
    pop ax
    pop bp
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MESSAGE_DISK_ERROR: db 'disk error!', 0
MESSAGE_READ_START: db 'Reading...', 0
MESSAGE_READ_END: db 'Read successfully!', 0

TOTAL_SECTOR_COUNT: dw 1024
SECTOR_IDX: db 2
HEAD_IDX: db 0
TRACK_IDX: db 0

BOOT_DRIVE: db 0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Bootloader Signature
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

times 510 - ($ - $$) db 0x00
dw 0xAA55