; stage2_entry.asm
bits 16

global stage2_entry
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

stage2_entry:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    lgdt [gdt_descriptor]

    ; protected mode enable
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; far jumpでCSを32bitコードセグメントへ更新
    jmp CODE_SEG:protected_entry

bits 32

protected_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    call kernel_main

.hang:
    hlt
    jmp .hang

align 8

gdt_start:
    dq 0

; code segment: base=0, limit=4GB, executable, readable
gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

; data segment: base=0, limit=4GB, writable
gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
