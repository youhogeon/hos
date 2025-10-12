[BITS 64]

SECTION .text

global reloadCS, reloadDS, kReadTSC, kHlt, kTestAndSet, kSetTS, kClearTS

reloadCS:
	pop rax
	movzx rcx, di
	push rcx
	push rax
	retfq

reloadDS:
	mov ax, di
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	ret

kReadTSC:
    push rdx
    
    rdtsc
    
    shl rdx, 32
    or rax, rdx
    
    pop rdx
    ret

kHlt:
    hlt
    hlt
    ret

kTestAndSet:
    ; PARAM:
    ;     QWORD* pdwDestination
	;     QWORD qwCompare
	;     QWORD qwExchange

    mov rax, rsi
    
    lock cmpxchg byte [ rdi ], dl   
    je .EQUAL

	.NOTEQUAL:
		mov rax, 0x00
		ret
		
	.EQUAL:
		mov rax, 0x01
		ret

kSetTS:
	push rax

	mov rax, cr0
	or rax, 0x8
	mov cr0, rax

	pop rax
	ret

kClearTS:
	clts
	ret