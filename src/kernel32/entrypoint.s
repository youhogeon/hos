[ORG 0x10000]
[BITS 16]

SECTION .text

jmp 0x1000:BEGIN

BEGIN:
    cli

    mov ax, 0x1000
    mov ds, ax
    mov es, ax

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

        jmp dword 0x18:PROTECTED_MODE_BEGIN


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PROTECTED MODE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[BITS 32]
PROTECTED_MODE_BEGIN:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0xFFFC
    mov ebp, 0xFFFC

    jmp dword 0x18:0x10200


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

    K64_CODE_DESCRIPTOR:     
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A         ; P=1, DPL=0, Code Segment, Execute/Read 
        db 0xAF         ; G=1, D=0, L=0, Limit=0
        db 0x00
        
    K64_DATA_DESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92         ; P=1, DPL=0, Data Segment, Read/Write
        db 0xAF         ; G=1, D=0, L=0, Limit=0
        db 0x00

    K32_CODE_DESCRIPTOR:     
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x9A         ; P=1, DPL=0, Code Segment, Execute/Read 
        db 0xCF         ; G=1, D=1, L=0, Limit=0
        db 0x00
        
    K32_DATA_DESCRIPTOR:
        dw 0xFFFF
        dw 0x0000
        db 0x00
        db 0x92         ; P=1, DPL=0, Data Segment, Read/Write
        db 0xCF         ; G=1, D=1, L=0, Limit=0
        db 0x00
GDTEND:

times 512 - ($ - $$) db 0x00