[ORG 0x7C00]
[BITS 16]

SECTION .text

jmp 0x0000:BEGIN

BEGIN:
    cli
    mov [BOOT_DRIVE], dl

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov ax, 0x9000
    mov ss, ax
    mov sp, 0xFFFE
    mov bp, sp
    sti


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Clear Display
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CLEAR_DISPLAY:
    mov ax, 0xB800
    mov es, ax

    xor di, di

    .CLEARLOOP:
        mov byte[ es: di ], 0
        mov byte[ es: di + 1 ], 0x03

        add di, 2

        cmp di, 80 * 25 * 2
        jl .CLEARLOOP

    ; move cursor to (0,0)
    mov ah, 0x02
    mov bh, 0
    mov dh, 0
    mov dl, 0
    int 0x10

    ; LOGO
    push MESSAGE_LOGO
    call PRINT_MESSAGE
    add sp, 2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Load OS into 0x10000
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RESETDISK:
    mov dl, [BOOT_DRIVE]
    xor ax, ax
    mov ah, 0x08
    int 0x13
    jc HANDLE_DISK_ERROR

    inc dh
    mov [GEOM_HEADS], dh

    mov al, cl
    and al, 0x3F
    mov [GEOM_SPT], al

READDATA:
    ; print
    push MESSAGE_READ_START
    call PRINT_MESSAGE
    add sp, 2

    mov si, 0x1000
    mov di, word[ OS_SECTOR_SIZE ]

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

        ; increase sector
        inc byte[ SECTOR_IDX ]
        mov al, byte[ SECTOR_IDX ]
        cmp al, byte [GEOM_SPT]
        jle .READLOOP

        ; increase head
        mov byte[ SECTOR_IDX ], 1
        inc byte[ HEAD_IDX ]
        mov al, byte[ HEAD_IDX ]
        cmp al, byte[ GEOM_HEADS ]
        jl .READLOOP

        ; increase track
        mov byte[ HEAD_IDX ], 0
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
    pusha

    mov si, word[bp + 4]

    .MESSAGELOOP:
        mov al, byte[ si ]
        cmp al, 0
        je  .MESSAGELOOP_END

        mov ah, 0x0E
        mov bh, 0
        int 0x10

        inc si
        jmp .MESSAGELOOP

    .MESSAGELOOP_END:
        ; CRLF
        mov ax, 0x0E0D
        mov bh, 0
        int 0x10
        mov ax, 0x0E0A
        int 0x10

    popa
    pop bp
    ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MESSAGE_LOGO: db '===============================', 0x0D, 0x0A, '============ H O S ============', 0x0D, 0x0A, '===============================', 0x0D, 0x0A, 0
MESSAGE_DISK_ERROR: db 'disk error!', 0
MESSAGE_READ_START: db 'Kernel32 Loading...', 0
MESSAGE_READ_END: db 'Kernel32 Loaded.', 0

OS_SECTOR_SIZE: dw 1024

GEOM_SPT: db 0
GEOM_HEADS: db 0

SECTOR_IDX: db 2
HEAD_IDX: db 0
TRACK_IDX: db 0

BOOT_DRIVE: db 0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Bootloader Signature
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

times 510 - ($ - $$) db 0x00
dw 0xAA55