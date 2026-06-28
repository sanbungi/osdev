# BIOS機能

https://wiki.osdev.org/BIOS

AHレジスタとINTオペコードの2つを使って、BIOSの機能を呼び出すことができる。
たとえばAH = 0x0e INT = 0x10 をセットすると、ALレジスタにある文字を1文字表示する。

```
  mov ah, 0x0e
  mov al, 'A'
  int 0x10
```
