#include "lib.h"

#define COM1 0x3F8
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef __builtin_va_list va_list;

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

static inline void outb(u16 port, u8 value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
  u8 value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static volatile u16 *const VGA = (volatile u16 *)0xB8000;

struct printk_sink {
  u8 target;
  u8 color;
  u32 x;
  u32 y;
};

void serial_init(void) {
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

void vga_clear(void) {
  for (u32 i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    VGA[i] = ((u16)0x0F << 8) | ' ';
  }
}

void vga_putc_at(char c, u8 color, u32 x, u32 y) {
  if (x < VGA_WIDTH && y < VGA_HEIGHT) {
    VGA[y * VGA_WIDTH + x] = ((u16)color << 8) | (u8)c;
  }
}

static char hex_digit(u8 value, int uppercase) {
  value &= 0x0F;
  if (value < 10) {
    return (char)('0' + value);
  }

  return (char)((uppercase ? 'A' : 'a') + value - 10);
}

static void printk_emit_char(struct printk_sink *sink, char c) {
  if (sink->target & PRINTK_SERIAL) {
    serial_putc(c);
  }

  if ((sink->target & PRINTK_VGA) == 0) {
    return;
  }

  if (c == '\r') {
    sink->x = 0;
    return;
  }

  if (c == '\n') {
    sink->x = 0;
    if (sink->y + 1 < VGA_HEIGHT) {
      sink->y++;
    }
    return;
  }

  vga_putc_at(c, sink->color, sink->x, sink->y);
  sink->x++;
  if (sink->x >= VGA_WIDTH) {
    sink->x = 0;
    if (sink->y + 1 < VGA_HEIGHT) {
      sink->y++;
    }
  }
}

static void printk_emit_string(struct printk_sink *sink, const char *s) {
  if (!s) {
    s = "(null)";
  }

  while (*s) {
    printk_emit_char(sink, *s++);
  }
}

static void printk_emit_padding(struct printk_sink *sink, char pad, u32 count) {
  for (u32 i = 0; i < count; i++) {
    printk_emit_char(sink, pad);
  }
}

static void printk_emit_u32(struct printk_sink *sink, u32 value, u32 width,
                            char pad) {
  char buf[10];
  u32 len = 0;

  if (value == 0) {
    buf[len++] = '0';
  } else {
    while (value != 0 && len < sizeof(buf)) {
      buf[len++] = (char)('0' + (value % 10));
      value /= 10;
    }
  }

  if (width > len) {
    printk_emit_padding(sink, pad, width - len);
  }

  while (len > 0) {
    printk_emit_char(sink, buf[--len]);
  }
}

static void printk_emit_i32(struct printk_sink *sink, int value, u32 width,
                            char pad) {
  u32 magnitude;

  if (value < 0) {
    printk_emit_char(sink, '-');
    magnitude = (u32)(-(value + 1)) + 1;
    if (width > 0) {
      width--;
    }
  } else {
    magnitude = (u32)value;
  }

  printk_emit_u32(sink, magnitude, width, pad);
}

static void printk_emit_hex32(struct printk_sink *sink, u32 value, u32 width,
                              char pad, int uppercase) {
  char buf[8];
  u32 len = 0;

  if (value == 0) {
    buf[len++] = '0';
  } else {
    while (value != 0 && len < sizeof(buf)) {
      buf[len++] = hex_digit((u8)value, uppercase);
      value >>= 4;
    }
  }

  if (width > len) {
    printk_emit_padding(sink, pad, width - len);
  }

  while (len > 0) {
    printk_emit_char(sink, buf[--len]);
  }
}

static const char *printk_parse_width(const char *fmt, char *pad, u32 *width) {
  *pad = ' ';
  *width = 0;

  if (*fmt == '0') {
    *pad = '0';
    fmt++;
  }

  while (*fmt >= '0' && *fmt <= '9') {
    *width = (*width * 10) + (u32)(*fmt - '0');
    fmt++;
  }

  return fmt;
}

void printk(u8 target, u8 color, u32 x, u32 y, const char *fmt, ...) {
  struct printk_sink sink;
  va_list ap;

  sink.target = target;
  sink.color = color;
  sink.x = x;
  sink.y = y;

  va_start(ap, fmt);

  while (*fmt) {
    char pad;
    u32 width;

    if (*fmt != '%') {
      printk_emit_char(&sink, *fmt++);
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      printk_emit_char(&sink, *fmt++);
      continue;
    }

    fmt = printk_parse_width(fmt, &pad, &width);

    switch (*fmt) {
    case 's':
      printk_emit_string(&sink, va_arg(ap, const char *));
      break;
    case 'c':
      printk_emit_char(&sink, (char)va_arg(ap, int));
      break;
    case 'd':
      printk_emit_i32(&sink, va_arg(ap, int), width, pad);
      break;
    case 'u':
      printk_emit_u32(&sink, va_arg(ap, u32), width, pad);
      break;
    case 'x':
      printk_emit_hex32(&sink, va_arg(ap, u32), width, pad, 0);
      break;
    case 'X':
      printk_emit_hex32(&sink, va_arg(ap, u32), width, pad, 1);
      break;
    case 'p': {
      void *ptr = va_arg(ap, void *);
      printk_emit_string(&sink, "0x");
      printk_emit_hex32(&sink, (u32)ptr, 8, '0', 1);
      break;
    }
    case '\0':
      fmt--;
      break;
    default:
      printk_emit_char(&sink, '%');
      printk_emit_char(&sink, *fmt);
      break;
    }

    if (*fmt) {
      fmt++;
    }
  }

  va_end(ap);
}
