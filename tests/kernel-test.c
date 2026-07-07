#include "../lib.h"

#define EXCEPTION_DE 0
#define EXCEPTION_DB 1

enum ktest_active {
  KTEST_NONE = 0,
  KTEST_DEBUG_EXCEPTION,
  KTEST_DIVIDE_BY_ZERO,
};

static enum ktest_active active_test = KTEST_NONE;
static u32 debug_exception_seen;
static u32 divide_by_zero_started;

static void ktest_halt(void) {
  for (;;) {
    __asm__ volatile("cli; hlt");
  }
}

static void ktest_pass(const char *name) {
  printk("KTEST event=pass name=%s\r\n", name);
  qemu_exit_success();
  ktest_halt();
}

static void ktest_fail_assert(const char *name, const char *expr, u32 line) {
  printk("KTEST event=fail name=%s reason=assert expr=%s line=%u\r\n", name,
         expr, line);
  qemu_exit_failure();
  ktest_halt();
}

static void ktest_fail_assert_u32_eq(const char *name, const char *expr,
                                     u32 expected, u32 actual, u32 line) {
  printk("KTEST event=fail name=%s reason=assert_eq expr=%s expected=0x%08X "
         "actual=0x%08X line=%u\r\n",
         name, expr, expected, actual, line);
  qemu_exit_failure();
  ktest_halt();
}

#define KTEST_ASSERT(name, expr)                                               \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ktest_fail_assert((name), #expr, __LINE__);                              \
    }                                                                          \
  } while (0)

#define KTEST_ASSERT_U32_EQ(name, expected, actual)                            \
  do {                                                                         \
    u32 ktest_expected = (u32)(expected);                                      \
    u32 ktest_actual = (u32)(actual);                                          \
    if (ktest_actual != ktest_expected) {                                      \
      ktest_fail_assert_u32_eq((name), #actual, ktest_expected, ktest_actual,  \
                               __LINE__);                                      \
    }                                                                          \
  } while (0)

static void ktest_fail_unexpected_exception(u32 vector, u32 eip, u32 cs,
                                            u32 eflags) {
  printk("KTEST event=fail name=exception reason=unexpected vector=0x%02X "
         "eip=0x%08X cs=0x%08X eflags=0x%08X\r\n",
         vector, eip, cs, eflags);
  qemu_exit_failure();
  ktest_halt();
}

void ktest_on_exception(u32 vector, u32 eip, u32 cs, u32 eflags) {
  if (vector == EXCEPTION_DB && active_test == KTEST_DEBUG_EXCEPTION) {
    debug_exception_seen = 1;
    return;
  }

  if (vector == EXCEPTION_DE && active_test == KTEST_DIVIDE_BY_ZERO &&
      divide_by_zero_started) {
    printk("KTEST event=pass name=divide_by_zero vector=0x%02X "
           "eip=0x%08X cs=0x%08X eflags=0x%08X\r\n",
           vector, eip, cs, eflags);
    qemu_exit_success();
    ktest_halt();
  }

  ktest_fail_unexpected_exception(vector, eip, cs, eflags);
}

void test_debug_exception(void) {
  printk("KTEST event=start name=debug_exception vector=0x%02X\r\n",
         EXCEPTION_DB);

  active_test = KTEST_DEBUG_EXCEPTION;
  debug_exception_seen = 0;

  __asm__ volatile("pushfl\n\t"
                   "orl $0x100, (%%esp)\n\t"
                   "popfl\n\t"
                   "nop"
                   :
                   :
                   : "cc", "memory");

  if (debug_exception_seen) {
    printk("KTEST event=pass name=debug_exception vector=0x%02X\r\n",
           EXCEPTION_DB);
    active_test = KTEST_NONE;
    return;
  }

  printk("KTEST event=fail name=debug_exception reason=no_exception\r\n");
  qemu_exit_failure();
  ktest_halt();
}

void test_divide_by_zero(void) {
  volatile u32 dividend = 1;
  volatile u32 divisor = 0;

  printk("KTEST event=start name=divide_by_zero vector=0x%02X\r\n",
         EXCEPTION_DE);

  active_test = KTEST_DIVIDE_BY_ZERO;
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

void test_memory_write(void) {
  volatile u32 *addr = (volatile u32 *)0x70000;
  const u32 expected = 0xCAFEBABE;

  printk("KTEST event=start name=memory_write addr=0x%08X value=0x%08X\r\n",
         (u32)addr, expected);

  *addr = expected;

  KTEST_ASSERT_U32_EQ("memory_write", expected, *addr);
  ktest_pass("memory_write");
}
