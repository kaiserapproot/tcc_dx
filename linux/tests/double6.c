/* Linux counterpart of dev/test/repro_double6.c (SysV: 8 xmm args). */
#include <stdio.h>
static double r[8];
static void test8(double a, double b, double c, double d,
                  double e, double f, double g, double h)
{
    r[0] = a; r[1] = b; r[2] = c; r[3] = d;
    r[4] = e; r[5] = f; r[6] = g; r[7] = h;
}
int main(void)
{
    float fa = -15.96f, fb = 15.96f, fc = -1.815f, fd = 21.025f;
    float fe = -45.72f, ff = 45.64f;
    int i;
    test8(fa, fb + 0.0f, fc, fd, fe, ff, 7.0, 8.0);
    if (r[0] != (double)fa || r[1] != (double)fb || r[2] != (double)fc
        || r[3] != (double)fd || r[4] != (double)fe || r[5] != (double)ff
        || r[6] != 7.0 || r[7] != 8.0) {
        for (i = 0; i < 8; i++) printf("%f\n", r[i]);
        return 1;
    }
    return 0;
}
