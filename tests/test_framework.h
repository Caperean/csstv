#ifndef CSSTV_TEST_FRAMEWORK_H
#define CSSTV_TEST_FRAMEWORK_H

#include <cstdio>
#include <string>

namespace csstv_test {

/* Global pass/fail counters, shared across all test translation units. */
inline int &failure_count()
{
    static int count = 0;
    return count;
}

inline int &check_count()
{
    static int count = 0;
    return count;
}

inline void report_check(bool passed, const char *expr, const char *file, int line)
{
    ++check_count();
    if (!passed)
    {
        ++failure_count();
        std::fprintf(stderr, "  [FAIL] %s:%d: CHECK(%s)\n", file, line, expr);
    }
}

} /* namespace csstv_test */

#define CHECK(expr) \
    csstv_test::report_check((expr), #expr, __FILE__, __LINE__)

#define RUN_TEST(fn)                              \
    do                                             \
    {                                              \
        std::printf("---- running %s ----\n", #fn); \
        fn();                                     \
    } while (0)

#endif /* CSSTV_TEST_FRAMEWORK_H */
