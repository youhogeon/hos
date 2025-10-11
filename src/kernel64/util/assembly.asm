[BITS 64]

SECTION .text

global reloadCS, reloadDS, kReadTSC, kHlt

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
