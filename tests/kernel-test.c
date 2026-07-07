#include "runner.h"

void test_debug_exception(void) {
  ktest_start_exception("debug_exception", EXCEPTION_DB);

  ktest_expect_debug_exception();

  __asm__ volatile("pushfl\n\t"
                   "orl $0x100, (%%esp)\n\t"
                   "popfl\n\t"
                   "nop"
                   :
                   :
                   : "cc", "memory");

  KTEST_ASSERT("debug_exception", ktest_debug_exception_seen());
  ktest_pass("debug_exception");
}

void test_divide_by_zero(void) {
  volatile u32 dividend = 1;
  volatile u32 divisor = 0;

  ktest_start_exception("divide_by_zero", EXCEPTION_DE);

  ktest_expect_divide_by_zero();

  __asm__ volatile("xorl %%edx, %%edx\n\t"
                   "divl %1"
                   : "+a"(dividend)
                   : "m"(divisor)
                   : "edx", "cc");

  ktest_fail("divide_by_zero", "no_exception");
}

void test_memory_write(void) {
  volatile u32 *addr = (volatile u32 *)0x70000;
  const u32 expected = 0xCAFEBABE;

  ktest_start("memory_write");

  *addr = expected;

  KTEST_ASSERT_U32_EQ("memory_write", expected, *addr);
  ktest_pass("memory_write");
}
