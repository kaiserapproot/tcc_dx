/* lib/va_list.c + include/stdarg.h on x86-64 SysV: integer, double and
   stack-passed arguments, va_copy, and a va_list handed to libc. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static long isum(int n, ...)
{
    va_list ap;
    long s = 0;
    va_start(ap, n);
    while (n--)
        s += va_arg(ap, int);
    va_end(ap);
    return s;
}

static double mixed(int n, ...)
{
    va_list ap, cp;
    double s = 0;
    int i;
    va_start(ap, n);
    va_copy(cp, ap);
    for (i = 0; i < n; i++)
        s += va_arg(ap, double) * va_arg(ap, long);
    for (i = 0; i < n; i++) {
        s -= va_arg(cp, double);
        (void)va_arg(cp, long);
    }
    va_end(cp);
    va_end(ap);
    return s;
}

static int fmt(char *buf, size_t size, const char *f, ...)
{
    va_list ap;
    int r;
    va_start(ap, f);
    r = vsnprintf(buf, size, f, ap);
    va_end(ap);
    return r;
}

int main(void)
{
    char buf[128];

    if (isum(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55)
        return 1;
    /* 10 doubles + 10 longs: both register files spill to the stack */
    if (mixed(10, 1.0, 1L, 2.0, 2L, 3.0, 3L, 4.0, 4L, 5.0, 5L, 6.0, 6L,
              7.0, 7L, 8.0, 8L, 9.0, 9L, 10.0, 10L) != 385.0 - 55.0)
        return 2;
    fmt(buf, sizeof buf, "%d %s %.2f %ld %c", 42, "tpp", 2.5, 1234567890123L, 'x');
    if (strcmp(buf, "42 tpp 2.50 1234567890123 x") != 0)
        return 3;
    return 0;
}
