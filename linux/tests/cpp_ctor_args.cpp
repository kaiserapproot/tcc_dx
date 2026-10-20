/* global and local objects whose ctor takes more than 6 integer and
   8 floating arguments (stack-passed under SysV) */
struct P {
    long s;
    double d;
    P(int a, int b, int c, int d0, int e, int f, int g, int h,
      double x1, double x2, double x3, double x4, double x5,
      double x6, double x7, double x8, double x9, double x10)
    {
        s = a + b * 2 + c * 3 + d0 * 4 + e * 5 + f * 6 + g * 7 + h * 8;
        d = x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 * 100 + x10 * 1000;
    }
};
P g_p(1, 2, 3, 4, 5, 6, 7, 8, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3);
int main()
{
    P l(1, 2, 3, 4, 5, 6, 7, 8, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3);
    if (g_p.s != 204 || g_p.d != 3208.0) return 1;
    if (l.s != 204 || l.d != 3208.0) return 2;
    return 0;
}
