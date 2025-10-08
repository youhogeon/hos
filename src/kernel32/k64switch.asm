[BITS 32]

global kReadCPUID, kSwitchTo64

SECTION .text
kReadCPUID:
    ; PARAM:
    ;     DWORD dwEAX,
    ;     DWORD* pdwEAX, pdwEBX, pdwECX, pdwEDX

    push ebp
    mov ebp, esp
    pushad

    ; Execute
    mov eax, [ebp + 8]
    cpuid

    ; Store results
    mov edi, [ebp + 12]
    mov [edi], eax
    mov edi, [ebp + 16]
    mov [edi], ebx
    mov edi, [ebp + 20]
    mov [edi], ecx
    mov edi, [ebp + 24]
    mov [edi], edx

    popad
    mov esp, ebp
    pop ebp
    ret

kSwitchTo64:
    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; set PML4 table address
    mov eax, 0x100000
    mov cr3, eax

    ; Enable LME
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    ; Enable Cache, Paging
    mov eax, cr0
    or eax, 0xE0000000
    xor eax, 0x60000000
    mov cr0, eax

    ; Far jump to 64bit code segment
    jmp 0x08:0x200000