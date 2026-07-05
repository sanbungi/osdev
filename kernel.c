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
#define IDT_SIZE 256

#define KERNEL_CODE_SELECTOR 0x08

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

#define IRQ_KEYBOARD 1
#define INT_KEYBOARD 0x21

#define PS2_DATA 0x60
#define PS2_STATUS 0x64

struct idt_entry {
  u16 offset_low;
  u16 selector;
  u8 zero;
  u8 type_attr;
  u16 offset_high;
} __attribute__((packed));

struct idt_ptr {
  u16 limit;
  u32 base;
} __attribute__((packed));

static struct idt_entry idt[IDT_SIZE];

extern void keyboard_irq_stub(void);

static void io_wait(void) { outb(0x80, 0); }

static void idt_set_gate(u8 index, u32 handler, u16 selector, u8 type_attr) {
  idt[index].offset_low = handler & 0xFFFF;
  idt[index].selector = selector;
  idt[index].zero = 0;
  idt[index].type_attr = type_attr;
  idt[index].offset_high = (handler >> 16) & 0xFFFF;
}

static void idt_load(void) {
  struct idt_ptr ptr;

  ptr.limit = sizeof(idt) - 1;
  ptr.base = (u32)&idt;

  __asm__ volatile("lidt %0" : : "m"(ptr));
}

static void idt_init(void) {
  for (u32 i = 0; i < IDT_SIZE; i++) {
    idt_set_gate(i, 0, 0, 0);
  }

  // 0x8E = present, ring 0, 32-bit interrupt gate
  idt_set_gate(INT_KEYBOARD, (u32)keyboard_irq_stub, KERNEL_CODE_SELECTOR,
               0x8E);

  idt_load();
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

static char hex_digit(u8 value) {
  value &= 0x0F;
  return value < 10 ? (char)('0' + value) : (char)('A' + value - 10);
}

static void serial_print_hex8(u8 value) {
  serial_putc(hex_digit(value >> 4));
  serial_putc(hex_digit(value));
}

static void serial_print(const char *s) {
  while (*s) {
    serial_putc(*s++);
  }
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

static void pic_remap(void) {
  u8 master_mask = inb(PIC1_DATA);
  u8 slave_mask = inb(PIC2_DATA);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
  io_wait();

  // Master PIC IRQ0-7  -> interrupt 0x20-0x27
  // Slave  PIC IRQ8-15 -> interrupt 0x28-0x2F
  outb(PIC1_DATA, 0x20);
  io_wait();
  outb(PIC2_DATA, 0x28);
  io_wait();

  // Tell Master PIC that Slave PIC is at IRQ2
  outb(PIC1_DATA, 0x04);
  io_wait();

  // Tell Slave PIC its cascade identity
  outb(PIC2_DATA, 0x02);
  io_wait();

  outb(PIC1_DATA, ICW4_8086);
  io_wait();
  outb(PIC2_DATA, ICW4_8086);
  io_wait();

  outb(PIC1_DATA, master_mask);
  outb(PIC2_DATA, slave_mask);
}

static void pic_set_mask(u8 irq) {
  u16 port;
  u8 value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port) | (1 << irq);
  outb(port, value);
}

static void pic_clear_mask(u8 irq) {
  u16 port;
  u8 value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  value = inb(port) & ~(1 << irq);
  outb(port, value);
}

static void pic_send_eoi(u8 irq) {
  if (irq >= 8) {
    outb(PIC2_COMMAND, PIC_EOI);
  }

  outb(PIC1_COMMAND, PIC_EOI);
}

static char scancode_to_ascii(u8 scancode) {
  static const char table[128] = {
      [0x02] = '1', [0x03] = '2',  [0x04] = '3', [0x05] = '4', [0x06] = '5',
      [0x07] = '6', [0x08] = '7',  [0x09] = '8', [0x0A] = '9', [0x0B] = '0',

      [0x10] = 'q', [0x11] = 'w',  [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
      [0x15] = 'y', [0x16] = 'u',  [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',

      [0x1E] = 'a', [0x1F] = 's',  [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
      [0x23] = 'h', [0x24] = 'j',  [0x25] = 'k', [0x26] = 'l',

      [0x2C] = 'z', [0x2D] = 'x',  [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
      [0x31] = 'n', [0x32] = 'm',

      [0x39] = ' ', [0x1C] = '\n',
  };

  if (scancode < 128) {
    return table[scancode];
  }

  return 0;
}

static u32 keyboard_x = 0;
static u32 keyboard_y = 23;

void keyboard_irq_handler(void) {
  u8 scancode = inb(PS2_DATA);

  serial_print("keyboard irq scancode=0x");
  serial_print_hex8(scancode);
  serial_print("\r\n");

  // key release は無視
  if ((scancode & 0x80) == 0) {
    char c = scancode_to_ascii(scancode);

    if (c) {
      serial_print("key char=");
      serial_putc(c);
      serial_print("\r\n");

      if (c == '\n') {
        keyboard_x = 0;
        keyboard_y++;
        if (keyboard_y >= 25) {
          keyboard_y = 23;
        }
      } else {
        vga_putc_at(c, 0x0A, keyboard_x, keyboard_y);
        keyboard_x++;
        if (keyboard_x >= 80) {
          keyboard_x = 0;
          keyboard_y++;
          if (keyboard_y >= 25) {
            keyboard_y = 23;
          }
        }
      }
    }
  }

  pic_send_eoi(IRQ_KEYBOARD);
}

static void keyboard_interrupt_init(void) {
  serial_print("Initializing IDT\r\n");
  idt_init();

  serial_print("Remapping PIC\r\n");
  pic_remap();

  // いったん全部マスクしてから IRQ1 だけ有効化
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  pic_clear_mask(IRQ_KEYBOARD);

  serial_print("Enabling CPU interrupts\r\n");
  __asm__ volatile("sti");
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

static void dump_memory_bytes(u32 start_address, u32 byte_count) {
  const volatile u8 *p = (const volatile u8 *)start_address;

  for (u32 offset = 0; offset < byte_count; offset += 16) {
    serial_print_hex32(start_address + offset);
    serial_print(": ");

    for (u32 i = 0; i < 16; i++) {
      if (offset + i < byte_count) {
        serial_print_hex8(p[offset + i]);
        serial_putc(' ');
      } else {
        serial_print("   ");
      }
    }

    serial_print(" |");

    for (u32 i = 0; i < 16 && offset + i < byte_count; i++) {
      u8 c = p[offset + i];

      if (c >= 0x20 && c <= 0x7E) {
        serial_putc((char)c);
      } else {
        serial_putc('.');
      }
    }

    serial_print("|\r\n");
  }
}

static void memory_write_demo(void) {
  volatile u32 *addr = (volatile u32 *)0x70000;

  serial_print("write_and_dump_demo\r\n");

  serial_print("before dump:\r\n");
  dump_memory_bytes(0x6FFF0, 0x40);

  *addr = 0xCAFEBABE;

  serial_print("after dump:\r\n");
  dump_memory_bytes(0x6FFF0, 0x40);

  serial_print("read as u32: ");
  serial_print_hex32(*addr);
  serial_print("\r\n");
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

  dump_memory_map(memmap, entry_count);
  show_memory_map_on_vga(memmap, entry_count);

  memory_write_demo();

  vga_print_at("Keyboard IRQ enabled. Type keys.", 0x0F, 0, 22);
  keyboard_interrupt_init();

  for (;;) {
    __asm__ volatile("hlt");
  }
}
