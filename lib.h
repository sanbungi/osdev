#ifndef LIB_H
#define LIB_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define PRINTK_SERIAL 0x01
#define PRINTK_VGA 0x02
#define PRINTK_BOTH (PRINTK_SERIAL | PRINTK_VGA)

void serial_init(void);
void vga_clear(void);
void vga_putc_at(char c, u8 color, u32 x, u32 y);
void printk(u8 target, u8 color, u32 x, u32 y, const char *fmt, ...);

#endif
