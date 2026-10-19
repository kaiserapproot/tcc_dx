#include <stdarg.h>
#include "abi.h"

long f_int8(int a, int b, int c, int d, int e, int f, int g, int h)
{ return a + b * 2L + c * 3L + d * 4L + e * 5L + f * 6L + g * 7L + h * 8L; }

double f_dbl10(double x1, double x2, double x3, double x4, double x5,
               double x6, double x7, double x8, double x9, double x10)
{ return x1 + x2 * 2 + x3 * 3 + x4 * 4 + x5 * 5 + x6 * 6 + x7 * 7 + x8 * 8
         + x9 * 9 + x10 * 10; }

double f_mixed(int i1, double d1, int i2, double d2, int i3, double d3,
               int i4, double d4, int i5, double d5, int i6, double d6,
               int i7, double d7, double d8, double d9, long l8)
{ return i1 + i2 * 2 + i3 * 3 + i4 * 4 + i5 * 5 + i6 * 6 + i7 * 7 + l8 * 8
         + (d1 + d2 * 2 + d3 * 3 + d4 * 4 + d5 * 5 + d6 * 6 + d7 * 7
            + d8 * 8 + d9 * 9) / 1000.0; }

long v_sum_i(int n, ...)
{ va_list ap; long s = 0; int i; va_start(ap, n);
  for (i = 0; i < n; i++) s += (long)va_arg(ap, int) * (i + 1);
  va_end(ap); return s; }

double v_sum_d(int n, ...)
{ va_list ap; double s = 0; int i; va_start(ap, n);
  for (i = 0; i < n; i++) s += va_arg(ap, double) * (i + 1);
  va_end(ap); return s; }

double v_mixed(int n, ...)
{ va_list ap; double s = 0; int i; va_start(ap, n);
  for (i = 0; i < n; i++) { s += va_arg(ap, int) * 1000.0;
                            s += va_arg(ap, double); }
  va_end(ap); return s; }

long v_copy(int n, ...)
{ va_list ap, cp; long a = 0, b = 0; int i; va_start(ap, n);
  va_copy(cp, ap);
  for (i = 0; i < n; i++) a += va_arg(ap, int);           /* ap first ... */
  for (i = 0; i < n; i++) b += va_arg(cp, int) * 100;     /* ... then cp */
  va_end(cp); va_end(ap); return a + b; }

long p_s8(S8 s) { return s.a * 10L + s.b; }
long p_s16(S16 s) { return s.a * 10 + s.b; }
double p_sfi(SFI s) { return s.f * 10 + s.i; }
double p_sdd(SDD s) { return s.a * 10 + s.b; }
double p_sdl(SDL s) { return s.d * 10 + s.l; }
long p_s24(S24 s) { return s.a * 100 + s.b * 10 + s.c; }
long p_s3(S3 s) { return s.c[0] * 100 + s.c[1] * 10 + s.c[2]; }
long p_spill(int a, int b, int c, int d, int e, int f, S16 s, S24 m, SDD dd)
{ return a + b + c + d + e + f + s.a * 10 + s.b * 100 + m.a * 1000
         + m.b * 10000 + m.c * 100000 + (long)(dd.a * 1000000 + dd.b * 10000000); }

S8 r_s8(int a, int b) { S8 s; s.a = a; s.b = b; return s; }
S16 r_s16(long a, long b) { S16 s; s.a = a; s.b = b; return s; }
SFI r_sfi(float f, int i) { SFI s; s.f = f; s.i = i; return s; }
SDD r_sdd(double a, double b) { SDD s; s.a = a; s.b = b; return s; }
SDL r_sdl(double d, long l) { SDL s; s.d = d; s.l = l; return s; }
S24 r_s24(long a, long b, long c) { S24 s; s.a = a; s.b = b; s.c = c; return s; }
S3 r_s3(int a, int b, int c) { S3 s; s.c[0] = a; s.c[1] = b; s.c[2] = c; return s; }

double call_cb(cb_t cb)
{ S16 s; SDD d; s.a = 3; s.b = 4; d.a = 0.25; d.b = 0.5;
  return cb(1, 0.5, 2, 1.5, 3, 2.5, 4, 3.5, 5, 4.5, s, d); }
