#include "../lib.h"

static void ktest_halt(void) {
  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}

#ifdef KTEST_DIVIDE_BY_ZERO
static u32 divide_by_zero_started;

void ktest_handle_divide_error(u32 eip, u32 cs, u32 eflags) {
  if (!divide_by_zero_started) {
    printk("KTEST event=fail name=divide_by_zero reason=unexpected_de "
           "eip=0x%08X cs=0x%08X eflags=0x%08X\r\n",
           eip, cs, eflags);
    qemu_exit_failure();
    ktest_halt();
  }

  printk("KTEST event=pass name=divide_by_zero eip=0x%08X cs=0x%08X "
         "eflags=0x%08X\r\n",
         eip, cs, eflags);
  qemu_exit_success();
  ktest_halt();
}

void test_divide_by_zero(void) {
  volatile u32 dividend = 1;
  volatile u32 divisor = 0;

  printk("KTEST event=start name=divide_by_zero\r\n");
  divide_by_zero_started = 1;

  __asm__ volatile("xorl %%edx, %%edx\n\t"
                   "divl %1"
                   : "+a"(dividend)
                   : "m"(divisor)
                   : "edx", "cc");

  printk("KTEST event=fail name=divide_by_zero reason=no_exception\r\n");
  qemu_exit_failure();
  ktest_halt();
}
#else
void ktest_handle_divide_error(u32 eip, u32 cs, u32 eflags) {
  printk("KTEST event=fail name=unknown reason=unexpected_de eip=0x%08X "
         "cs=0x%08X eflags=0x%08X\r\n",
         eip, cs, eflags);
  qemu_exit_failure();
  ktest_halt();
}
#endif
