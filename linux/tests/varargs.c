#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int sum(int n, ...)
{
    va_list ap;
    int i, s = 0;
    va_start(ap, n);
    for (i = 0; i < n; i++)
        s += va_arg(ap, int);
    va_end(ap);
    return s;
}

static double dsum(int n, ...)
{
    va_list ap;
    int i;
    double s = 0;
    va_start(ap, n);
    for (i = 0; i < n; i++)
        s += va_arg(ap, double);
    va_end(ap);
    return s;
}

int main(void)
{
    char buf[64];
    if (sum(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55) return 1;
    if (dsum(10, 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.) != 55.) return 2;
    snprintf(buf, sizeof buf, "%d %s %.2f", 42, "x", 3.5);
    if (strcmp(buf, "42 x 3.50") != 0) return 3;
    return 0;
}
