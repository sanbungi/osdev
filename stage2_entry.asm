; まだ16bit
bits 16
; リンカ向け宣言
global stage2_entry
extern kernel_main

CODE_SEG equ 0x08
DATA_SEG equ 0x10

; BIOS E820 entries are copied here as an array of 32-bit words.
; One entry is 5 dwords:
;   [0] base_low
;   [1] base_high
;   [2] length_low
;   [3] length_high
;   [4] type
; biosから取得したメモリマップを0x5000に保存
MEMMAP_ADDR equ 0x5000
MEMMAP_ENTRY_SIZE equ 20 ;1エントリ
MEMMAP_MAX_ENTRIES equ 32 ;32件まで保存

stage2_entry:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    ; スタックは0x7000から
    mov sp, 0x7000
    sti

    ; BIOS呼び出しはリアルモードで行う
    call get_memory_map
    cli

    ; GDTの場所とサイズをCPUに教える命令
    lgdt [gdt_descriptor]

    ; CR0はCPU制御レジスタ。プロテクトモードに入る
    mov eax, cr0
    ; EAXの最下位ビットを1に、bit0はPEモード有効
    or eax, 1
    ; 変更した値を戻す
    mov cr0, eax

    ; far jumpでCSを32bitコードセグメントへ更新しprotected_entryを実行
    jmp CODE_SEG:protected_entry

; Collect the BIOS E820 memory map into MEMMAP_ADDR.
; This keeps only the first 20 bytes per entry so kernel.c can read it
; as a plain u32 array.
get_memory_map:
    ;一旦レジスタを保存
    pushad
    push es

    ; EE820はES:DIが指す場所に書き込む
    xor ax, ax
    ; ES=0
    mov es, ax
    xor ebx, ebx
    xor bp, bp

    ; DI=5000なので、ES:DI = 0000:5000
    mov di, MEMMAP_ADDR

.next:
    ; int 0x15の呼び出し準備
    mov eax, 0xE820
    mov edx, 0x534D4150 ; 'SMAP' というASCII署名
    mov ecx, MEMMAP_ENTRY_SIZE ; 5要素 * 4列 = 20
    int 0x15
    jc .done

    ; 正しく動いたならeaxに SMAPが入る
    cmp eax, 0x534D4150
    jne .done
    ; 返り値のサイズチェック
    cmp ecx, MEMMAP_ENTRY_SIZE
    jb .done

    ; bp++;
    inc bp
    ; DI+=20;
    add di, MEMMAP_ENTRY_SIZE
    ; if (bp == MEMMAP_MAX_ENTRIES) done;
    cmp bp, MEMMAP_MAX_ENTRIES
    jae .done

    ;まだ続きがあるならEBXに0が以外が入る
    test ebx, ebx
    jne .next

.done:
    ; 取得件数を書く
    mov [memmap_count], bp
    ; 保存していたレジスタを戻す
    pop es
    popad
    ret

; 32ビット世界突入
bits 32

protected_entry:
    mov ax, DATA_SEG ;ax = 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; このGDTでは base=0, limit=4GBなので、アドレスはほぼ物理アドレス

    ; 先頭は0x90000
    mov esp, 0x90000

    ; Cカーネル呼び出し
    ; kernel_main((const u32 *)MEMMAP_ADDR, memmap_count)
    push dword [memmap_count]
    push dword MEMMAP_ADDR
    call kernel_main
    add esp, 8

.hang:
    ; CPU停止
    hlt
    jmp .hang

align 4
memmap_count:
    dd 0

align 8

; 最初は空にする
gdt_start:
    dq 0

; code segment: base=0, limit=4GB, 実行可能, 読み込み可能
; GDTの1番目なので、1 * 8 = 0x08 (CODE_SEG equ 0x08)
gdt_code:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

; data segment: base=0, limit=4GB, 書き込み可能
; 二番目だから、2 * 8 = 0x10 (DATA_SEG equ 0x10)
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

section .note.GNU-stack noalloc noexec nowrite progbits
