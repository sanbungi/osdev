#include "runner.h"

void test_debug_exception(void);
void test_divide_by_zero(void);
void test_memory_write(void);

enum ktest_active {
  KTEST_NONE = 0,
  KTEST_DEBUG_EXCEPTION,
  KTEST_DIVIDE_BY_ZERO,
};

static enum ktest_active active_test = KTEST_NONE;
static u32 debug_exception_seen;
static u32 divide_by_zero_started;

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

void ktest_start(const char *name) {
  printk("KTEST event=start name=%s\r\n", name);
}

void ktest_start_exception(const char *name, u32 vector) {
  printk("KTEST event=start name=%s vector=0x%02X\r\n", name, vector);
}

void ktest_pass(const char *name) {
  printk("KTEST event=pass name=%s\r\n", name);
  qemu_exit_success();
  ktest_halt();
}

void ktest_pass_exception(const char *name, u32 vector, u32 eip, u32 cs,
                          u32 eflags) {
  printk("KTEST event=pass name=%s vector=0x%02X "
         "eip=0x%08X cs=0x%08X eflags=0x%08X\r\n",
         name, vector, eip, cs, eflags);
  qemu_exit_success();
  ktest_halt();
}

void ktest_fail(const char *name, const char *reason) {
  printk("KTEST event=fail name=%s reason=%s\r\n", name, reason);
  qemu_exit_failure();
  ktest_halt();
}

void ktest_fail_unexpected_exception(u32 vector, u32 eip, u32 cs, u32 eflags) {
  printk("KTEST event=fail name=exception reason=unexpected vector=0x%02X "
         "eip=0x%08X cs=0x%08X eflags=0x%08X\r\n",
         vector, eip, cs, eflags);
  qemu_exit_failure();
  ktest_halt();
}

void ktest_fail_assert(const char *name, const char *expr, u32 line) {
  printk("KTEST event=fail name=%s reason=assert expr=%s line=%u\r\n", name,
         expr, line);
  qemu_exit_failure();
  ktest_halt();
}

void ktest_fail_assert_u32_eq(const char *name, const char *expr, u32 expected,
                              u32 actual, u32 line) {
  printk("KTEST event=fail name=%s reason=assert_eq expr=%s expected=0x%08X "
         "actual=0x%08X line=%u\r\n",
         name, expr, expected, actual, line);
  qemu_exit_failure();
  ktest_halt();
}

void ktest_expect_debug_exception(void) {
  active_test = KTEST_DEBUG_EXCEPTION;
  debug_exception_seen = 0;
}

int ktest_debug_exception_seen(void) { return debug_exception_seen; }

void ktest_expect_divide_by_zero(void) {
  active_test = KTEST_DIVIDE_BY_ZERO;
  divide_by_zero_started = 1;
}

void ktest_on_exception(u32 vector, u32 eip, u32 cs, u32 eflags) {
  if (vector == EXCEPTION_DB && active_test == KTEST_DEBUG_EXCEPTION) {
    debug_exception_seen = 1;
    return;
  }

  if (vector == EXCEPTION_DE && active_test == KTEST_DIVIDE_BY_ZERO &&
      divide_by_zero_started) {
    ktest_pass_exception("divide_by_zero", vector, eip, cs, eflags);
  }

  ktest_fail_unexpected_exception(vector, eip, cs, eflags);
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

  if (streq(KTEST_NAME, "memory_write")) {
    test_memory_write();
    return;
  }

  printk("KTEST event=fail name=%s reason=unknown_test\r\n", KTEST_NAME);
  qemu_exit_failure();
  ktest_halt();
}
