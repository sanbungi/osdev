#include "../lib.h"

#ifdef KTEST_DIVIDE_BY_ZERO
void test_divide_by_zero(void);
#endif

static void ktest_halt(void) {
  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}

void run_kernel_test(void) {
#ifdef KTEST_DIVIDE_BY_ZERO
  test_divide_by_zero();
#else
  printk("KTEST event=fail name=none reason=no_test_selected\r\n");
  qemu_exit_failure();
  ktest_halt();
#endif
}
