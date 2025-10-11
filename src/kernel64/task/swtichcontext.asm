[BITS 64]

%include "./util/macro.inc"

SECTION .text

global kSwitchContext

kSwitchContext:
    ; PARAM:
    ;     CONTEXT* pstCurrentContext
    ;     CONTEXT* pstNextContext

    push rbp
    mov rbp, rsp
    
    ; if Current Context == NULL
    pushfq
    cmp rdi, 0
    je .LoadContext 
    popfq

    push rax
    
    mov ax, ss
    mov qword[ rdi + ( 23 * 8 ) ], rax

    mov rax, rbp
    add rax, 16
    mov qword[ rdi + ( 22 * 8 ) ], rax
    
    pushfq
    pop rax
    mov qword[ rdi + ( 21 * 8 ) ], rax

    mov ax, cs
    mov qword[ rdi + ( 20 * 8 ) ], rax
    
    mov rax, qword[ rbp + 8 ]
    mov qword[ rdi + ( 19 * 8 ) ], rax

    pop rax
    pop rbp
    
    add rdi, ( 19 * 8 )
    mov rsp, rdi
    sub rdi, ( 19 * 8 )
    
    K_SAVE_CONTEXT

    .LoadContext:
        mov rsp, rsi

        K_RESTORE_CONTEXT

        iretq
