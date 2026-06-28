; boot.asm
bits 16
org 0x7c00

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 32
%endif

STAGE2_ADDR equ 0x8000

start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    sti

    mov [boot_drive], dl

    mov si, msg_loading
    call bios_print

    ; BIOS Extended Read
    ; read LBA 1.. into 0000:8000
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

    mov si, msg_ok
    call bios_print

    ; stage2へジャンプ
    jmp 0x0000:STAGE2_ADDR

disk_error:
    mov si, msg_disk_error
    call bios_print

.hang:
    hlt
    jmp .hang

bios_print:
.next:
    lodsb
    cmp al, 0
    je .done

    mov ah, 0x0e
    int 0x10
    jmp .next

.done:
    ret

boot_drive:
    db 0

msg_loading:
    db "Loading stage2...", 13, 10, 0

msg_ok:
    db "Jumping to stage2...", 13, 10, 0

msg_disk_error:
    db "Disk read error", 13, 10, 0

; Disk Address Packet for INT 13h AH=42h
dap:
    db 16                 ; packet size
    db 0                  ; reserved
    dw STAGE2_SECTORS     ; number of sectors
    dw STAGE2_ADDR        ; offset
    dw 0x0000             ; segment
    dq 1                  ; starting LBA. LBA 0 is boot sector

times 510 - ($ - $$) db 0
dw 0xaa55
