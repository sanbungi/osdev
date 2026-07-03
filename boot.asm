bits 16
; BIOSは0x7c00から実行する?
org 0x7c00

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 32
%endif

STAGE2_ADDR equ 0x8000

start:
    ; 割り込み禁止
    cli

    ; 初期化
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    ; スタックを7x00に
    mov sp, 0x7c00

    ; 割り込みを許可
    sti

    ; BIOSが起動元ドライブ番号0x00、0x80…をDL(に入れてくれるので、それを保存)
    mov [boot_drive], dl

    ; SIにmsg_loadingのデータアドレスを入れる
    mov si, msg_loading
    call bios_print

    ; BIOS Extended Read
    ; read LBA 1.. into 0000:8000
    mov ah, 0x42
    mov dl, [boot_drive]

    ; dapは Disk Address Packet
    mov si, dap

    ; biosのディスク機能を呼ぶ
    ; AH=0x42 拡張読み込み
    ; DL = BIOSが起動元ドライブ
    ; DS:SI = dap
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

; biosの機能で1文字ずつ表示
bios_print:
.next:
    lodsb
    ; if (al == 0 ); done
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
; LBA 1 から32セクタを 0000:8000 に読み込む
dap:
    db 16                 ; dapのサイズ 16byte
    db 0                  ; 予約領域
    dw STAGE2_SECTORS     ; 読み込むセクタ 32
    dw STAGE2_ADDR        ; 読み込み先のオフセット 0x8000
    dw 0x0000             ; 読み込み先のセグメント 0000:8000
    dq 1                  ; 開始LBA 1から読み出す、0はこのboot loader

times 510 - ($ - $$) db 0
dw 0xaa55
