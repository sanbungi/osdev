bits 32

global keyboard_irq_stub
extern keyboard_irq_handler

keyboard_irq_stub:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call keyboard_irq_handler

    pop gs
    pop fs
    pop es
    pop ds

    popa
    iretd

section .note.GNU-stack noalloc noexec nowrite progbits
