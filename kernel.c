#include "lib.h"

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
extern void exception0_stub(void);
extern void exception6_stub(void);

#ifdef KERNEL_TEST
void run_kernel_test(void);
void ktest_on_exception(u32 vector, u32 eip, u32 cs, u32 eflags);
#endif

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
  idt_set_gate(0, (u32)exception0_stub, KERNEL_CODE_SELECTOR, 0x8E);
  idt_set_gate(6, (u32)exception6_stub, KERNEL_CODE_SELECTOR, 0x8E);

  // 0x8E = present, ring 0, 32-bit interrupt gate
  idt_set_gate(INT_KEYBOARD, (u32)keyboard_irq_stub, KERNEL_CODE_SELECTOR,
               0x8E);

  idt_load();
}

struct exception_frame {
  u32 edi;
  u32 esi;
  u32 ebp;
  u32 esp;
  u32 ebx;
  u32 edx;
  u32 ecx;
  u32 eax;

  u32 eip;
  u32 cs;
  u32 eflags;
};

// IDT 0x00 #DE Divide Error が発生した場合のハンドラ
void exception0_handler(struct exception_frame *frame) {
#ifdef KERNEL_TEST
  ktest_on_exception(0, frame->eip, frame->cs, frame->eflags);
#endif

  printk("\r\n#DE Divide Error\r\n");
  printk("eip=0x%08X\r\n", frame->eip);
  printk("cs=0x%08X eflags=0x%08X\r\n", frame->cs, frame->eflags);

  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}

// IDT 0x06 #UD Invalid Opcode が発生した場合のハンドラ
void exception6_handler(struct exception_frame *frame) {
#ifdef KERNEL_TEST
  ktest_on_exception(6, frame->eip, frame->cs, frame->eflags);
#endif

  printk("\r\n#UD Invalid Opcode\r\n");
  printk("eip=0x%08X\r\n", frame->eip);
  printk("cs=0x%08X eflags=0x%08X\r\n", frame->cs, frame->eflags);

  for (;;) {
    __asm__ volatile("cli; hlt");
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

  printk_serial("keyboard irq scancode=0x");
  printk_serial("%02X", scancode);
  printk_serial("\r\n");

  // key release は無視
  if ((scancode & 0x80) == 0) {
    char c = scancode_to_ascii(scancode);

    if (c) {
      printk_serial("key char=");
      printk_serial("%c", c);
      printk_serial("\r\n");

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
  printk("Initializing IDT\r\n");
  idt_init();

  printk("Remapping PIC\r\n");
  pic_remap();

  // いったん全部マスクしてから IRQ1 だけ有効化
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);

  pic_clear_mask(IRQ_KEYBOARD);

  printk("Enabling CPU interrupts\r\n");
  __asm__ volatile("sti");
}

void kernel_main(const u32 *memmap, u32 entry_count) {
  serial_init();

  printk("Entered C kernel_main()\r\n");

#ifdef KERNEL_TEST
  idt_init();
  run_kernel_test();
#endif

  vga_clear();
  printk_at(0x0A, 0, 0, "Stage1 -> Stage2 -> Protected Mode -> C");
  printk_at(0x0F, 0, 1, "Memory map passed to kernel_main()");
  printk_set_cursor(0, 2);

  printk("Memory map pointer: ");
  printk("0x%08X", (u32)memmap);
  printk("\r\n");

  dump_memory_map(memmap, entry_count);
  show_memory_map_on_vga(memmap, entry_count);

  dump_memory_map(memmap, entry_count);
  show_memory_map_on_vga(memmap, entry_count);

  printk_at(0x0F, 0, 22, "Keyboard IRQ enabled. Type keys.");
  keyboard_interrupt_init();

  for (;;) {
    __asm__ volatile("hlt");
  }
}
