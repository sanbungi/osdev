// kernel.c

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define COM1 0x3F8

static inline void outb(u16 port, u8 value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
  u8 value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static void serial_init(void) {
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);
}

static int serial_ready(void) { return inb(COM1 + 5) & 0x20; }

static void serial_putc(char c) {
  while (!serial_ready()) {
  }

  outb(COM1, (u8)c);
}

static void serial_print(const char *s) {
  while (*s) {
    serial_putc(*s++);
  }
}

static volatile u16 *const VGA = (volatile u16 *)0xB8000;

static void vga_clear(void) {
  for (u32 i = 0; i < 80 * 25; i++) {
    VGA[i] = ((u16)0x0F << 8) | ' ';
  }
}

static void vga_putc_at(char c, u8 color, u32 x, u32 y) {
  VGA[y * 80 + x] = ((u16)color << 8) | (u8)c;
}

static void vga_print_at(const char *s, u8 color, u32 x, u32 y) {
  while (*s) {
    vga_putc_at(*s, color, x, y);
    x++;
    s++;
  }
}

void kernel_main(void) {
  serial_init();

  serial_print("Entered C kernel_main()\r\n");

  vga_clear();
  vga_print_at("Hello from C kernel_main() Yatta!", 0x0F, 0, 0);
  vga_print_at("Stage1 -> Stage2 -> Protected Mode -> C", 0x0A, 0, 1);

  serial_print("VGA text written\r\n");

  for (;;) {
    __asm__ volatile("hlt");
  }
}
