[BITS 64]

SECTION .text

global reloadCS, reloadDS, setKernelStack

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