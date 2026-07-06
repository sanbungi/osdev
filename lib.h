#ifndef LIB_H
#define LIB_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

void serial_init(void);
void vga_clear(void);
void vga_putc_at(char c, u8 color, u32 x, u32 y);
void printk(const char *fmt, ...);
void printk_serial(const char *fmt, ...);
void printk_at(u8 color, u32 x, u32 y, const char *fmt, ...);
void printk_set_cursor(u32 x, u32 y);

#endif
