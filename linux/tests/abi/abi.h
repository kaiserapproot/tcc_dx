/* SysV x86_64 ABI cross check between gcc and tpp objects.
   abi_impl.c (callee) and abi_check.c (caller) are built by different
   compilers in run_tests.sh Phase 6. */
typedef struct { int a, b; } S8;
typedef struct { long a, b; } S16;
typedef struct { float f; int i; } SFI;
typedef struct { double a, b; } SDD;
typedef struct { double d; long l; } SDL;
typedef struct { long a, b, c; } S24;           /* memory class */
typedef struct { char c[3]; } S3;

long f_int8(int a, int b, int c, int d, int e, int f, int g, int h);
double f_dbl10(double x1, double x2, double x3, double x4, double x5,
               double x6, double x7, double x8, double x9, double x10);
double f_mixed(int i1, double d1, int i2, double d2, int i3, double d3,
               int i4, double d4, int i5, double d5, int i6, double d6,
               int i7, double d7, double d8, double d9, long l8);
long v_sum_i(int n, ...);
double v_sum_d(int n, ...);
double v_mixed(int n, ...);          /* pairs of (int, double) */
long v_copy(int n, ...);             /* va_copy: both lists read separately */

long p_s8(S8 s);
long p_s16(S16 s);
double p_sfi(SFI s);
double p_sdd(SDD s);
double p_sdl(SDL s);
long p_s24(S24 s);
long p_s3(S3 s);
long p_spill(int a, int b, int c, int d, int e, int f, S16 s, S24 m, SDD dd);

S8 r_s8(int a, int b);
S16 r_s16(long a, long b);
SFI r_sfi(float f, int i);
SDD r_sdd(double a, double b);
SDL r_sdl(double d, long l);
S24 r_s24(long a, long b, long c);
S3 r_s3(int a, int b, int c);

typedef double (*cb_t)(int, double, int, double, int, double, int, double,
                       int, double, S16, SDD);
double call_cb(cb_t cb);             /* callee calls back into the caller */
