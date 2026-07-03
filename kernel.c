// kernel.c

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define COM1 0x3F8
#define E820_WORDS_PER_ENTRY 5
#define E820_TYPE_USABLE 1

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

static char hex_digit(u8 value) {
  value &= 0x0F;
  return value < 10 ? (char)('0' + value) : (char)('A' + value - 10);
}

static void serial_print_hex32_raw(u32 value) {
  for (int shift = 28; shift >= 0; shift -= 4) {
    serial_putc(hex_digit((u8)(value >> shift)));
  }
}

static void serial_print_hex32(u32 value) {
  serial_print("0x");
  serial_print_hex32_raw(value);
}

static void serial_print_hex64(u32 high, u32 low) {
  serial_print("0x");
  serial_print_hex32_raw(high);
  serial_putc('_');
  serial_print_hex32_raw(low);
}

static void serial_print_u32(u32 value) {
  char buf[10];
  u32 len = 0;

  if (value == 0) {
    serial_putc('0');
    return;
  }

  while (value != 0 && len < sizeof(buf)) {
    buf[len++] = (char)('0' + (value % 10));
    value /= 10;
  }

  while (len > 0) {
    serial_putc(buf[--len]);
  }
}

static void serial_print_e820_type(u32 type) {
  if (type == E820_TYPE_USABLE) {
    serial_print("usable");
  } else if (type == 2) {
    serial_print("reserved");
  } else if (type == 3) {
    serial_print("acpi");
  } else if (type == 4) {
    serial_print("nvs");
  } else if (type == 5) {
    serial_print("bad");
  } else {
    serial_print("type ");
    serial_print_u32(type);
  }
}

static volatile u16 *const VGA = (volatile u16 *)0xB8000;

static void vga_clear(void) {
  for (u32 i = 0; i < 80 * 25; i++) {
    VGA[i] = ((u16)0x0F << 8) | ' ';
  }
}

static void vga_putc_at(char c, u8 color, u32 x, u32 y) {
  if (x < 80 && y < 25) {
    VGA[y * 80 + x] = ((u16)color << 8) | (u8)c;
  }
}

static void vga_print_at(const char *s, u8 color, u32 x, u32 y) {
  while (*s) {
    vga_putc_at(*s, color, x, y);
    x++;
    s++;
  }
}

static void vga_print_u32_at(u32 value, u8 color, u32 x, u32 y) {
  char buf[10];
  u32 len = 0;

  if (value == 0) {
    vga_putc_at('0', color, x, y);
    return;
  }

  while (value != 0 && len < sizeof(buf)) {
    buf[len++] = (char)('0' + (value % 10));
    value /= 10;
  }

  while (len > 0) {
    vga_putc_at(buf[--len], color, x++, y);
  }
}

static void vga_print_hex32_at(u32 value, u8 color, u32 x, u32 y) {
  vga_print_at("0x", color, x, y);
  x += 2;
  for (int shift = 28; shift >= 0; shift -= 4) {
    vga_putc_at(hex_digit((u8)(value >> shift)), color, x++, y);
  }
}

static void dump_memory_map(const u32 *memmap, u32 entry_count) {
  if (entry_count > 64) {
    serial_print("E820 entry count looks broken; clamp to 64\r\n");
    entry_count = 64;
  }

  serial_print("E820 memory map entries: ");
  serial_print_u32(entry_count);
  serial_print("\r\n");

  for (u32 i = 0; i < entry_count; i++) {
    const u32 *entry = memmap + i * E820_WORDS_PER_ENTRY;
    u32 base_low = entry[0];
    u32 base_high = entry[1];
    u32 length_low = entry[2];
    u32 length_high = entry[3];
    u32 type = entry[4];

    serial_print("  [");
    serial_print_u32(i);
    serial_print("] base=");
    serial_print_hex64(base_high, base_low);
    serial_print(" length=");
    serial_print_hex64(length_high, length_low);
    serial_print(" type=");
    serial_print_e820_type(type);
    serial_print("\r\n");
  }
}

static void show_memory_map_on_vga(const u32 *memmap, u32 entry_count) {
  vga_print_at("E820 entries:", 0x0F, 0, 2);
  vga_print_u32_at(entry_count, 0x0F, 14, 2);

  u32 shown = entry_count;
  for (u32 i = 0; i < shown; i++) {
    const u32 *entry = memmap + i * E820_WORDS_PER_ENTRY;
    u32 y = 4 + i;

    vga_print_u32_at(i, 0x0A, 0, y);
    vga_print_at(" base_lo=", 0x0F, 2, y);
    vga_print_hex32_at(entry[0], 0x0F, 11, y);
    vga_print_at(" len_lo=", 0x0F, 22, y);
    vga_print_hex32_at(entry[2], 0x0F, 30, y);
    vga_print_at(" type=", 0x0F, 41, y);
    vga_print_u32_at(entry[4], 0x0F, 47, y);
  }
}

void kernel_main(const u32 *memmap, u32 entry_count) {
  serial_init();

  serial_print("Entered C kernel_main()\r\n");

  vga_clear();
  vga_print_at("Stage1 -> Stage2 -> Protected Mode -> C", 0x0A, 0, 0);
  vga_print_at("Memory map passed to kernel_main()", 0x0F, 0, 1);

  serial_print("Memory map pointer: ");
  serial_print_hex32((u32)memmap);
  serial_print("\r\n");

  dump_memory_map(memmap, entry_count);
  show_memory_map_on_vga(memmap, entry_count);

  for (;;) {
    __asm__ volatile("hlt");
  }
}
