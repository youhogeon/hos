[ORG 0x10000]
[BITS 16]

SECTION .text

jmp 0x1000:BEGIN

BEGIN:
    cli

    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    ; get/set cursor position
    mov ah, 0x03
    mov bh, 0x00
    int 0x10

    mov [CURSOR_ROW], dh

    ; enable A20 line
    mov ax, 0x2401
    int 0x15

    jc .HANDLE_A20_ERROR
    jmp .HANDLE_A20_SUCCESS

    .HANDLE_A20_ERROR:
        in al, 0x92
        or al, 0x02
        and al, 0xFE
        out 0x92, al
    
    .HANDLE_A20_SUCCESS:
        ; jump to protected mode
        lgdt [ GDTR ]

        mov eax, 0x4000003B ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
        mov cr0, eax

        jmp dword 0x08:PROTECTED_MODE_BEGIN


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PROTECTED MODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[BITS 32]
PROTECTED_MODE_BEGIN:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0xFFFC
    mov ebp, 0xFFFC
    
    push MESSAGE_SWITCHED_TO_32
    call PRINT_MESSAGE
    add esp, 4

    jmp dword 0x08:0x10200


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; FUNCTIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PRINT_MESSAGE:
    push ebp
    mov ebp, esp
    pushad

    xor ebx, ebx
    mov bl, [CURSOR_ROW]
    mov cl, bl
    inc cl
    mov [CURSOR_ROW], cl
    imul ebx, ebx, 160

    mov edi, 0xB8000
    add edi, ebx
    mov esi, [ebp + 8]

    ; move cursor
    shr ebx, 1
    add ebx, 80

    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x3D5
    mov al, bl
    out dx, al

    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x3D5
    mov al, bh
    out dx, al

    .MESSAGE_LOOP:
        mov al, byte [esi]
        cmp al, 0
        je .MESSAGE_LOOP_END

        mov [edi], al
        add edi, 2
        inc esi
        jmp .MESSAGE_LOOP

    .MESSAGE_LOOP_END:

    popad
    pop ebp
    ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 8, db 0

dw 0
GDTR:
    dw GDTEND - GDT - 1  ; size
    dd GDT               ; address

GDT:
    NULLDescriptor:
        dw 0x0000       ; Limit [15:0]
        dw 0x0000       ; Base [15:0]
        db 0x00         ; Base [23:16]
        db 0x00         ; P, DPL, S, Type
        db 0x00         ; G, D, L, Limit[19:16]
        db 0x00         ; Base [31:24]  

    CODE_DESCRIPTOR:     
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A         ; P=1, DPL=0, Code Segment, Execute/Read 
        db 0xCF         ; G=1, D=1, L=0, Limit=0
        db 0x00
        
    DATA_DESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92         ; P=1, DPL=0, Data Segment, Read/Write
        db 0xCF         ; G=1, D=1, L=0, Limit=0
        db 0x00
GDTEND:

CURSOR_ROW: db 0
MESSAGE_SWITCHED_TO_32: db 'Switched to protected mode', 0

times 512 - ($ - $$) db 0x00