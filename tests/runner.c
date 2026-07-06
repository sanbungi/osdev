#include "../lib.h"

void test_debug_exception(void);
void test_divide_by_zero(void);

static int streq(const char *a, const char *b) {
  while (*a && *b && *a == *b) {
    a++;
    b++;
  }

  return *a == *b;
}

static void ktest_halt(void) {
  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}

void run_kernel_test(void) {
  if (streq(KTEST_NAME, "debug_exception")) {
    test_debug_exception();
    return;
  }

  if (streq(KTEST_NAME, "divide_by_zero")) {
    test_divide_by_zero();
    return;
  }

  printk("KTEST event=fail name=%s reason=unknown_test\r\n", KTEST_NAME);
  qemu_exit_failure();
  ktest_halt();
}
