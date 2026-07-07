#ifndef TESTS_RUNNER_H
#define TESTS_RUNNER_H

#include "../lib.h"

#define EXCEPTION_DE 0
#define EXCEPTION_DB 1

void ktest_start(const char *name);
void ktest_start_exception(const char *name, u32 vector);
void ktest_pass(const char *name);
void ktest_pass_exception(const char *name, u32 vector, u32 eip, u32 cs,
                          u32 eflags);
void ktest_fail(const char *name, const char *reason);
void ktest_fail_unexpected_exception(u32 vector, u32 eip, u32 cs, u32 eflags);
void ktest_fail_assert(const char *name, const char *expr, u32 line);
void ktest_fail_assert_u32_eq(const char *name, const char *expr, u32 expected,
                              u32 actual, u32 line);
void ktest_expect_debug_exception(void);
int ktest_debug_exception_seen(void);
void ktest_expect_divide_by_zero(void);

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

#endif
