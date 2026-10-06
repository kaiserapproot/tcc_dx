/* SSE / SSE2 intrinsic values, compared between TCC and MSVC.

   Every intrinsic of dev\include\xmmintrin.h and emmintrin.h is called on a
   set of inputs that includes the awkward cases (+-0, denormals, +-inf, NaN,
   integer limits, out-of-range shift counts and conversions), and the result
   bytes are printed in hex.  simd_values_expected.txt is this program's
   output when built by MSVC (cl /Od) - ..\manual\simd_golden_msvc.bat makes
   it - and ..\simd_gate.bat requires TCC's output, as C and as C++, to be the
   same text.

   rcp / rsqrt are approximations whose exact bits differ between CPU makers,
   so for those only the documented bound (relative error <= 1.5 * 2^-12) is
   printed.  _mm_getcsr prints the control bits only (the sticky exception
   flags depend on what ran before).

   The intrinsics are called directly: MSVC does not let their address be
   taken. */
#include <emmintrin.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>   /* _mm_stream_si64 */
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif

static void put_bytes(const char *name, int i, int j, const void *p, int n)
{
    const unsigned char *b = (const unsigned char *)p;
    int k;
    printf("%s %d %d:", name, i, j);
    for (k = 0; k < n; k++)
        printf(" %02x", b[k]);
    printf("\n");
}

static void put_int(const char *name, int i, int j, long long v)
{
    printf("%s %d %d: %lld\n", name, i, j, v);
}

/* ---- inputs ---- */

#define NF 4
#define ND 5
#define NI 5

static const unsigned int f_bits[NF][4] = {
    { 0x3fc00000, 0xc0100000, 0x00000000, 0x80000000 },   /* 1.5 -2.25 +0 -0 */
    { 0x7f61b1e6, 0x80011c6a, 0x7f800000, 0xff800000 },   /* 3e38 -1e-40 +inf -inf */
    { 0x7fc00000, 0x7fa00001, 0x3f800000, 0x40000000 },   /* qNaN sNaN 1 2 */
    { 0x3dcccccd, 0xc2c90000, 0x47800060, 0x2edbe6ff },   /* 0.1 -100.5 65536.75 1e-10 */
};

static const unsigned long long d_bits[ND][2] = {
    { 0x3ff8000000000000ULL, 0xc002000000000000ULL },     /* 1.5 -2.25 */
    { 0x0000000000000000ULL, 0x8000000000000000ULL },     /* +0 -0 */
    { 0x7fe1ccf385ebc8a0ULL, 0x8000000000000001ULL },     /* 1e308 -min denormal */
    { 0x7ff0000000000000ULL, 0x7ff8000000000000ULL },     /* +inf qNaN */
    { 0x3fb999999999999aULL, 0xc059200000000000ULL },     /* 0.1 -100.5 */
};

static const unsigned int i_words[NI][4] = {
    { 0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c },   /* bytes 0..15 */
    { 0x80000000, 0x7fffffff, 0xffffffff, 0x00000001 },   /* int32 limits */
    { 0x7fff8000, 0x0000ffff, 0xff000100, 0x3039ffff },   /* int16 limits */
    { 0x807f00ff, 0x7f80ff00, 0x01fe02fd, 0xc0408080 },   /* int8 limits */
    { 0x9e3779b9, 0x7f4a7c15, 0x1234abcd, 0xdeadbeef },   /* mixed */
};

static __m128 vf[NF];
static __m128d vd[ND];
static __m128i vi[NI];

static void init_inputs(void)
{
    int i;
    for (i = 0; i < NF; i++) memcpy(&vf[i], f_bits[i], 16);
    for (i = 0; i < ND; i++) memcpy(&vd[i], d_bits[i], 16);
    for (i = 0; i < NI; i++) memcpy(&vi[i], i_words[i], 16);
}

/* ---- loops over the inputs ---- */

#define FOR_F2(EXPR, NAME, T) do { int i, j; for (i = 0; i < NF; i++) for (j = 0; j < NF; j++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)
#define FOR_F1(EXPR, NAME, T) do { int i, j = 0; for (i = 0; i < NF; i++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)
#define FOR_D2(EXPR, NAME, T) do { int i, j; for (i = 0; i < ND; i++) for (j = 0; j < ND; j++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)
#define FOR_D1(EXPR, NAME, T) do { int i, j = 0; for (i = 0; i < ND; i++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)
#define FOR_I2(EXPR, NAME, T) do { int i, j; for (i = 0; i < NI; i++) for (j = 0; j < NI; j++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)
#define FOR_I1(EXPR, NAME, T) do { int i, j = 0; for (i = 0; i < NI; i++) { T r_ = EXPR; put_bytes(NAME, i, j, &r_, 16); } } while (0)

#define F2(fn) FOR_F2(fn(vf[i], vf[j]), #fn, __m128)
#define F1(fn) FOR_F1(fn(vf[i]), #fn, __m128)
#define D2(fn) FOR_D2(fn(vd[i], vd[j]), #fn, __m128d)
#define D1(fn) FOR_D1(fn(vd[i]), #fn, __m128d)
#define I2(fn) FOR_I2(fn(vi[i], vi[j]), #fn, __m128i)
#define FI_F(fn) do { int i, j; for (i = 0; i < NF; i++) for (j = 0; j < NF; j++) put_int(#fn, i, j, fn(vf[i], vf[j])); } while (0)
#define FI_D(fn) do { int i, j; for (i = 0; i < ND; i++) for (j = 0; j < ND; j++) put_int(#fn, i, j, fn(vd[i], vd[j])); } while (0)

static void sse_arith(void)
{
    F2(_mm_add_ss); F2(_mm_add_ps); F2(_mm_sub_ss); F2(_mm_sub_ps);
    F2(_mm_mul_ss); F2(_mm_mul_ps); F2(_mm_div_ss); F2(_mm_div_ps);
    F1(_mm_sqrt_ss); F1(_mm_sqrt_ps);
    F2(_mm_min_ss); F2(_mm_min_ps); F2(_mm_max_ss); F2(_mm_max_ps);
    F2(_mm_and_ps); F2(_mm_andnot_ps); F2(_mm_or_ps); F2(_mm_xor_ps);
}

static void sse_compare(void)
{
    F2(_mm_cmpeq_ss); F2(_mm_cmpeq_ps); F2(_mm_cmplt_ss); F2(_mm_cmplt_ps);
    F2(_mm_cmple_ss); F2(_mm_cmple_ps); F2(_mm_cmpgt_ss); F2(_mm_cmpgt_ps);
    F2(_mm_cmpge_ss); F2(_mm_cmpge_ps); F2(_mm_cmpneq_ss); F2(_mm_cmpneq_ps);
    F2(_mm_cmpnlt_ss); F2(_mm_cmpnlt_ps); F2(_mm_cmpnle_ss); F2(_mm_cmpnle_ps);
    F2(_mm_cmpngt_ss); F2(_mm_cmpngt_ps); F2(_mm_cmpnge_ss); F2(_mm_cmpnge_ps);
    F2(_mm_cmpord_ss); F2(_mm_cmpord_ps); F2(_mm_cmpunord_ss); F2(_mm_cmpunord_ps);
    FI_F(_mm_comieq_ss); FI_F(_mm_comilt_ss); FI_F(_mm_comile_ss);
    FI_F(_mm_comigt_ss); FI_F(_mm_comige_ss); FI_F(_mm_comineq_ss);
    FI_F(_mm_ucomieq_ss); FI_F(_mm_ucomilt_ss); FI_F(_mm_ucomile_ss);
    FI_F(_mm_ucomigt_ss); FI_F(_mm_ucomige_ss); FI_F(_mm_ucomineq_ss);
}

/* rcp / rsqrt: |approx - exact| <= 1.5 * 2^-12 * |exact| for normal inputs */
static int approx_ok(float approx, double exact)
{
    double err = approx - exact;
    if (err < 0) err = -err;
    if (exact < 0) exact = -exact;
    return err <= 1.5 / 4096.0 * exact;
}

static void sse_approx(void)
{
    static const float in[6] = { 1.0f, 2.0f, 3.0f, 0.1f, 1000.0f, 1.0e-6f };
    int k, lane;
    for (k = 0; k < 6; k++) {
        __m128 a = _mm_set_ps(in[k] * 4.0f, in[k] * 3.0f, in[k] * 2.0f, in[k]);
        __m128 rp = _mm_rcp_ps(a), rs = _mm_rsqrt_ps(a);
        __m128 rp1 = _mm_rcp_ss(a), rs1 = _mm_rsqrt_ss(a);
        int ok = 1;
        for (lane = 0; lane < 4; lane++) {
            double x = a.m128_f32[lane];
            ok &= approx_ok(rp.m128_f32[lane], 1.0 / x);
            ok &= approx_ok(rs.m128_f32[lane], 1.0 / sqrt(x));
        }
        ok &= approx_ok(rp1.m128_f32[0], 1.0 / a.m128_f32[0]) && rp1.m128_u32[1] == a.m128_u32[1]
              && rp1.m128_u32[2] == a.m128_u32[2] && rp1.m128_u32[3] == a.m128_u32[3];
        ok &= approx_ok(rs1.m128_f32[0], 1.0 / sqrt(a.m128_f32[0])) && rs1.m128_u32[3] == a.m128_u32[3];
        put_int("rcp_rsqrt_within_bound", k, 0, ok);
    }
}

static void sse_convert(void)
{
    static const float fs[8] = { 2.5f, 3.5f, -2.5f, -0.4f, 2147483648.0f, -2147483904.0f, 1.0e19f, 0.0f };
    static const int ints[4] = { 0, -7, 2147483647, (int)0x80000000 };
    static const long long lls[3] = { 0, -9007199254740993LL, 9223372036854775807LL };
    static const unsigned int modes[4] = { _MM_ROUND_NEAREST, _MM_ROUND_DOWN, _MM_ROUND_UP, _MM_ROUND_TOWARD_ZERO };
    unsigned int saved = _mm_getcsr();
    int k, m;
    for (m = 0; m < 4; m++) {
        _MM_SET_ROUNDING_MODE(modes[m]);
        for (k = 0; k < 8; k++) {
            __m128 a = _mm_set_ss(fs[k]);
            put_int("_mm_cvtss_si32", m, k, _mm_cvtss_si32(a));
            put_int("_mm_cvttss_si32", m, k, _mm_cvttss_si32(a));
            put_int("_mm_cvtss_si64", m, k, _mm_cvtss_si64(a));
            put_int("_mm_cvttss_si64", m, k, _mm_cvttss_si64(a));
        }
        for (k = 0; k < 4; k++) {
            __m128 r = _mm_cvtsi32_ss(vf[3], ints[k]);
            put_bytes("_mm_cvtsi32_ss", m, k, &r, 16);
        }
        for (k = 0; k < 3; k++) {
            __m128 r = _mm_cvtsi64_ss(vf[0], lls[k]);
            put_bytes("_mm_cvtsi64_ss", m, k, &r, 16);
        }
    }
    _mm_setcsr(saved);
    for (k = 0; k < NF; k++) {
        float f = _mm_cvtss_f32(vf[k]);
        put_bytes("_mm_cvtss_f32", k, 0, &f, 4);
    }
}

static void sse_memory(void)
{
    ALIGN16 float buf[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    ALIGN16 float out[8];
    __m64 m64v;
    __m128 r;
    int k;
    r = _mm_load_ps(buf); put_bytes("_mm_load_ps", 0, 0, &r, 16);
    r = _mm_loadu_ps(buf + 1); put_bytes("_mm_loadu_ps", 0, 0, &r, 16);
    r = _mm_load_ss(buf + 2); put_bytes("_mm_load_ss", 0, 0, &r, 16);
    r = _mm_load1_ps(buf + 3); put_bytes("_mm_load1_ps", 0, 0, &r, 16);
    r = _mm_loadr_ps(buf + 4); put_bytes("_mm_loadr_ps", 0, 0, &r, 16);
    memcpy(&m64v, buf + 6, 8);
    r = _mm_loadh_pi(vf[0], &m64v); put_bytes("_mm_loadh_pi", 0, 0, &r, 16);
    r = _mm_loadl_pi(vf[0], &m64v); put_bytes("_mm_loadl_pi", 0, 0, &r, 16);
    for (k = 0; k < NF; k++) {
        memset(out, 0xcc, sizeof out);
        _mm_store_ps(out, vf[k]); _mm_storeu_ps(out + 4, vf[k]); put_bytes("_mm_store_ps+storeu_ps", k, 0, out, 32);
        memset(out, 0xcc, sizeof out);
        _mm_store_ss(out + 1, vf[k]); _mm_store1_ps(out + 4, vf[k]); put_bytes("_mm_store_ss+store1_ps", k, 0, out, 32);
        memset(out, 0xcc, sizeof out);
        _mm_storer_ps(out, vf[k]); _mm_stream_ps(out + 4, vf[k]); put_bytes("_mm_storer_ps+stream_ps", k, 0, out, 32);
        memset(out, 0xcc, sizeof out);
        _mm_storeh_pi((__m64 *)out, vf[k]); _mm_storel_pi((__m64 *)(out + 3), vf[k]); put_bytes("_mm_storeh_pi+storel_pi", k, 0, out, 32);
    }
    _mm_sfence();
}

static void sse_set_move(void)
{
    __m128 r;
    r = _mm_setzero_ps(); put_bytes("_mm_setzero_ps", 0, 0, &r, 16);
    r = _mm_set_ss(-3.25f); put_bytes("_mm_set_ss", 0, 0, &r, 16);
    r = _mm_set1_ps(6.5f); put_bytes("_mm_set1_ps", 0, 0, &r, 16);
    r = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f); put_bytes("_mm_set_ps", 0, 0, &r, 16);
    r = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f); put_bytes("_mm_setr_ps", 0, 0, &r, 16);
    F2(_mm_move_ss); F2(_mm_movehl_ps); F2(_mm_movelh_ps); F2(_mm_unpackhi_ps); F2(_mm_unpacklo_ps);
    { int i; for (i = 0; i < NF; i++) put_int("_mm_movemask_ps", i, 0, _mm_movemask_ps(vf[i])); }
    FOR_F2(_mm_shuffle_ps(vf[i], vf[j], 0x00), "_mm_shuffle_ps_00", __m128);
    FOR_F2(_mm_shuffle_ps(vf[i], vf[j], 0x1b), "_mm_shuffle_ps_1b", __m128);
    FOR_F2(_mm_shuffle_ps(vf[i], vf[j], _MM_SHUFFLE(3, 1, 2, 0)), "_mm_shuffle_ps_d8", __m128);
    FOR_F2(_mm_shuffle_ps(vf[i], vf[j], 0xff), "_mm_shuffle_ps_ff", __m128);
    {
        __m128 r0 = vf[0], r1 = vf[1], r2 = vf[2], r3 = vf[3];
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
        put_bytes("_MM_TRANSPOSE4_PS", 0, 0, &r0, 16); put_bytes("_MM_TRANSPOSE4_PS", 1, 0, &r1, 16);
        put_bytes("_MM_TRANSPOSE4_PS", 2, 0, &r2, 16); put_bytes("_MM_TRANSPOSE4_PS", 3, 0, &r3, 16);
    }
    {
        unsigned int saved = _mm_getcsr();
        _MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
        put_int("_MM_GET_ROUNDING_MODE", 0, 0, _MM_GET_ROUNDING_MODE());
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        put_int("_mm_getcsr_control", 0, 0, _mm_getcsr() & ~0x3fu);
        _mm_setcsr(saved);
        put_int("_mm_getcsr_control", 1, 0, _mm_getcsr() & ~0x3fu);
    }
    {
        static float pf[64];
        _mm_prefetch((const char *)pf, _MM_HINT_T0);
        _mm_prefetch((const char *)pf, _MM_HINT_T1);
        _mm_prefetch((const char *)pf, _MM_HINT_T2);
        _mm_prefetch((const char *)pf, _MM_HINT_NTA);
        put_int("_mm_prefetch", 0, 0, 1);
    }
}

static void sse2_double(void)
{
    D2(_mm_add_sd); D2(_mm_add_pd); D2(_mm_sub_sd); D2(_mm_sub_pd);
    D2(_mm_mul_sd); D2(_mm_mul_pd); D2(_mm_div_sd); D2(_mm_div_pd);
    D2(_mm_sqrt_sd); D1(_mm_sqrt_pd);
    D2(_mm_min_sd); D2(_mm_min_pd); D2(_mm_max_sd); D2(_mm_max_pd);
    D2(_mm_and_pd); D2(_mm_andnot_pd); D2(_mm_or_pd); D2(_mm_xor_pd);
    D2(_mm_cmpeq_sd); D2(_mm_cmpeq_pd); D2(_mm_cmplt_sd); D2(_mm_cmplt_pd);
    D2(_mm_cmple_sd); D2(_mm_cmple_pd); D2(_mm_cmpgt_sd); D2(_mm_cmpgt_pd);
    D2(_mm_cmpge_sd); D2(_mm_cmpge_pd); D2(_mm_cmpneq_sd); D2(_mm_cmpneq_pd);
    D2(_mm_cmpnlt_sd); D2(_mm_cmpnlt_pd); D2(_mm_cmpnle_sd); D2(_mm_cmpnle_pd);
    D2(_mm_cmpngt_sd); D2(_mm_cmpngt_pd); D2(_mm_cmpnge_sd); D2(_mm_cmpnge_pd);
    D2(_mm_cmpord_sd); D2(_mm_cmpord_pd); D2(_mm_cmpunord_sd); D2(_mm_cmpunord_pd);
    FI_D(_mm_comieq_sd); FI_D(_mm_comilt_sd); FI_D(_mm_comile_sd);
    FI_D(_mm_comigt_sd); FI_D(_mm_comige_sd); FI_D(_mm_comineq_sd);
    FI_D(_mm_ucomieq_sd); FI_D(_mm_ucomilt_sd); FI_D(_mm_ucomile_sd);
    FI_D(_mm_ucomigt_sd); FI_D(_mm_ucomige_sd); FI_D(_mm_ucomineq_sd);
    D2(_mm_unpackhi_pd); D2(_mm_unpacklo_pd); D2(_mm_move_sd);
    { int i; for (i = 0; i < ND; i++) put_int("_mm_movemask_pd", i, 0, _mm_movemask_pd(vd[i])); }
    FOR_D2(_mm_shuffle_pd(vd[i], vd[j], 0), "_mm_shuffle_pd_0", __m128d);
    FOR_D2(_mm_shuffle_pd(vd[i], vd[j], 1), "_mm_shuffle_pd_1", __m128d);
    FOR_D2(_mm_shuffle_pd(vd[i], vd[j], 2), "_mm_shuffle_pd_2", __m128d);
    FOR_D2(_mm_shuffle_pd(vd[i], vd[j], 3), "_mm_shuffle_pd_3", __m128d);
}

static void sse2_convert(void)
{
    static const double ds[8] = { 2.5, 3.5, -2.5, -0.4, 2147483648.0, -2147483649.0, 1.0e19, 1.0e-320 };
    static const int ints[4] = { 0, -7, 2147483647, (int)0x80000000 };
    static const long long lls[3] = { 0, -9007199254740993LL, 9223372036854775807LL };
    static const unsigned int modes[4] = { _MM_ROUND_NEAREST, _MM_ROUND_DOWN, _MM_ROUND_UP, _MM_ROUND_TOWARD_ZERO };
    unsigned int saved = _mm_getcsr();
    int k, m;
    for (m = 0; m < 4; m++) {
        _MM_SET_ROUNDING_MODE(modes[m]);
        for (k = 0; k < 8; k++) {
            __m128d a = _mm_set_sd(ds[k]);
            put_int("_mm_cvtsd_si32", m, k, _mm_cvtsd_si32(a));
            put_int("_mm_cvttsd_si32", m, k, _mm_cvttsd_si32(a));
            put_int("_mm_cvtsd_si64", m, k, _mm_cvtsd_si64(a));
            put_int("_mm_cvttsd_si64", m, k, _mm_cvttsd_si64(a));
            { __m128 r = _mm_cvtsd_ss(vf[2], a); put_bytes("_mm_cvtsd_ss", m, k, &r, 16); }
        }
        for (k = 0; k < 4; k++) { __m128d r = _mm_cvtsi32_sd(vd[4], ints[k]); put_bytes("_mm_cvtsi32_sd", m, k, &r, 16); }
        for (k = 0; k < 3; k++) { __m128d r = _mm_cvtsi64_sd(vd[0], lls[k]); put_bytes("_mm_cvtsi64_sd", m, k, &r, 16); }
        { int i; for (i = 0; i < NI; i++) { __m128d r = _mm_cvtepi32_pd(vi[i]); put_bytes("_mm_cvtepi32_pd", m, i, &r, 16); } }
        { int i; for (i = 0; i < NI; i++) { __m128 r = _mm_cvtepi32_ps(vi[i]); put_bytes("_mm_cvtepi32_ps", m, i, &r, 16); } }
        { int i; for (i = 0; i < ND; i++) { __m128i r = _mm_cvtpd_epi32(vd[i]); put_bytes("_mm_cvtpd_epi32", m, i, &r, 16); } }
        { int i; for (i = 0; i < ND; i++) { __m128i r = _mm_cvttpd_epi32(vd[i]); put_bytes("_mm_cvttpd_epi32", m, i, &r, 16); } }
        { int i; for (i = 0; i < ND; i++) { __m128 r = _mm_cvtpd_ps(vd[i]); put_bytes("_mm_cvtpd_ps", m, i, &r, 16); } }
        { int i; for (i = 0; i < NF; i++) { __m128i r = _mm_cvtps_epi32(vf[i]); put_bytes("_mm_cvtps_epi32", m, i, &r, 16); } }
        { int i; for (i = 0; i < NF; i++) { __m128i r = _mm_cvttps_epi32(vf[i]); put_bytes("_mm_cvttps_epi32", m, i, &r, 16); } }
        { int i; for (i = 0; i < NF; i++) { __m128d r = _mm_cvtps_pd(vf[i]); put_bytes("_mm_cvtps_pd", m, i, &r, 16); } }
        { int i; for (i = 0; i < NF; i++) { __m128d r = _mm_cvtss_sd(vd[1], vf[i]); put_bytes("_mm_cvtss_sd", m, i, &r, 16); } }
    }
    _mm_setcsr(saved);
    { int i; for (i = 0; i < ND; i++) { double v = _mm_cvtsd_f64(vd[i]); put_bytes("_mm_cvtsd_f64", i, 0, &v, 8); } }
    { int i; for (i = 0; i < NI; i++) put_int("_mm_cvtsi128_si32", i, 0, _mm_cvtsi128_si32(vi[i])); }
    { int i; for (i = 0; i < NI; i++) put_int("_mm_cvtsi128_si64", i, 0, _mm_cvtsi128_si64(vi[i])); }
    { __m128i r = _mm_cvtsi32_si128(-5); put_bytes("_mm_cvtsi32_si128", 0, 0, &r, 16); }
    { __m128i r = _mm_cvtsi64_si128(-0x123456789LL); put_bytes("_mm_cvtsi64_si128", 0, 0, &r, 16); }
}

static void sse2_set_memory(void)
{
    ALIGN16 double db[4] = { 1.25, -2.5, 3.75, -5.0 };
    ALIGN16 double dout[4];
    ALIGN16 unsigned char ib[48];
    ALIGN16 unsigned char iout[48];
    __m128d d;
    __m128i r;
    int k;
    for (k = 0; k < 48; k++) ib[k] = (unsigned char)(k * 7 + 3);
    d = _mm_setzero_pd(); put_bytes("_mm_setzero_pd", 0, 0, &d, 16);
    d = _mm_set_sd(-7.5); put_bytes("_mm_set_sd", 0, 0, &d, 16);
    d = _mm_set1_pd(0.25); put_bytes("_mm_set1_pd", 0, 0, &d, 16);
    d = _mm_set_pd(1.0, 2.0); put_bytes("_mm_set_pd", 0, 0, &d, 16);
    d = _mm_setr_pd(1.0, 2.0); put_bytes("_mm_setr_pd", 0, 0, &d, 16);
    d = _mm_load_pd(db); put_bytes("_mm_load_pd", 0, 0, &d, 16);
    d = _mm_loadu_pd(db + 1); put_bytes("_mm_loadu_pd", 0, 0, &d, 16);
    d = _mm_load_sd(db + 2); put_bytes("_mm_load_sd", 0, 0, &d, 16);
    d = _mm_load1_pd(db + 3); put_bytes("_mm_load1_pd", 0, 0, &d, 16);
    d = _mm_loadr_pd(db + 2); put_bytes("_mm_loadr_pd", 0, 0, &d, 16);
    d = _mm_loadh_pd(vd[0], db + 1); put_bytes("_mm_loadh_pd", 0, 0, &d, 16);
    d = _mm_loadl_pd(vd[0], db + 3); put_bytes("_mm_loadl_pd", 0, 0, &d, 16);
    for (k = 0; k < ND; k++) {
        memset(dout, 0xcc, sizeof dout);
        _mm_store_pd(dout, vd[k]); _mm_storeu_pd(dout + 2, vd[k]); put_bytes("_mm_store_pd+storeu_pd", k, 0, dout, 32);
        memset(dout, 0xcc, sizeof dout);
        _mm_store_sd(dout + 1, vd[k]); _mm_store1_pd(dout + 2, vd[k]); put_bytes("_mm_store_sd+store1_pd", k, 0, dout, 32);
        memset(dout, 0xcc, sizeof dout);
        _mm_storer_pd(dout, vd[k]); _mm_stream_pd(dout + 2, vd[k]); put_bytes("_mm_storer_pd+stream_pd", k, 0, dout, 32);
        memset(dout, 0xcc, sizeof dout);
        _mm_storeh_pd(dout, vd[k]); _mm_storel_pd(dout + 3, vd[k]); put_bytes("_mm_storeh_pd+storel_pd", k, 0, dout, 32);
    }
    r = _mm_setzero_si128(); put_bytes("_mm_setzero_si128", 0, 0, &r, 16);
    r = _mm_set_epi64x(-2, 0x0123456789abcdefLL); put_bytes("_mm_set_epi64x", 0, 0, &r, 16);
    r = _mm_set_epi32(1, -2, 3, -4); put_bytes("_mm_set_epi32", 0, 0, &r, 16);
    r = _mm_set_epi16(1, -2, 3, -4, 5, -6, 7, -8); put_bytes("_mm_set_epi16", 0, 0, &r, 16);
    r = _mm_set_epi8(1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16); put_bytes("_mm_set_epi8", 0, 0, &r, 16);
    r = _mm_set1_epi64x(-0x1234LL); put_bytes("_mm_set1_epi64x", 0, 0, &r, 16);
    r = _mm_set1_epi32(-77); put_bytes("_mm_set1_epi32", 0, 0, &r, 16);
    r = _mm_set1_epi16(-300); put_bytes("_mm_set1_epi16", 0, 0, &r, 16);
    r = _mm_set1_epi8(-3); put_bytes("_mm_set1_epi8", 0, 0, &r, 16);
    r = _mm_setr_epi32(1, -2, 3, -4); put_bytes("_mm_setr_epi32", 0, 0, &r, 16);
    r = _mm_setr_epi16(1, -2, 3, -4, 5, -6, 7, -8); put_bytes("_mm_setr_epi16", 0, 0, &r, 16);
    r = _mm_setr_epi8(1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16); put_bytes("_mm_setr_epi8", 0, 0, &r, 16);
    r = _mm_load_si128((__m128i const *)ib); put_bytes("_mm_load_si128", 0, 0, &r, 16);
    r = _mm_loadu_si128((__m128i const *)(ib + 3)); put_bytes("_mm_loadu_si128", 0, 0, &r, 16);
    r = _mm_loadl_epi64((__m128i const *)(ib + 5)); put_bytes("_mm_loadl_epi64", 0, 0, &r, 16);
    for (k = 0; k < NI; k++) {
        memset(iout, 0xcc, sizeof iout);
        _mm_store_si128((__m128i *)iout, vi[k]); _mm_storeu_si128((__m128i *)(iout + 17), vi[k]);
        _mm_storel_epi64((__m128i *)(iout + 38), vi[k]);
        put_bytes("_mm_store_si128+storeu+storel", k, 0, iout, 48);
        memset(iout, 0xcc, sizeof iout);
        _mm_stream_si128((__m128i *)iout, vi[k]); _mm_stream_si32((int *)(iout + 20), vi[k].m128i_i32[1]);
        _mm_stream_si64((__int64 *)(iout + 32), vi[k].m128i_i64[1]);
        _mm_maskmoveu_si128(vi[k], vi[3], (char *)iout);
        put_bytes("_mm_stream+maskmoveu", k, 0, iout, 48);
    }
    _mm_clflush(iout);
    _mm_lfence();
    _mm_mfence();
    _mm_pause();
}

#define SH_I(fn, c) do { int i; for (i = 0; i < NI; i++) { __m128i r_ = fn(vi[i], c); put_bytes(#fn, i, c, &r_, 16); } } while (0)
#define SH_CONST(fn, c) do { int i; for (i = 0; i < NI; i++) { __m128i r_ = fn(vi[i], c); put_bytes(#fn, i, c, &r_, 16); } } while (0)

static void sse2_integer(void)
{
    static const int counts[10] = { 0, 1, 7, 15, 16, 31, 32, 63, 64, 255 };
    int c;
    I2(_mm_add_epi8); I2(_mm_add_epi16); I2(_mm_add_epi32); I2(_mm_add_epi64);
    I2(_mm_adds_epi8); I2(_mm_adds_epi16); I2(_mm_adds_epu8); I2(_mm_adds_epu16);
    I2(_mm_sub_epi8); I2(_mm_sub_epi16); I2(_mm_sub_epi32); I2(_mm_sub_epi64);
    I2(_mm_subs_epi8); I2(_mm_subs_epi16); I2(_mm_subs_epu8); I2(_mm_subs_epu16);
    I2(_mm_avg_epu8); I2(_mm_avg_epu16); I2(_mm_madd_epi16);
    I2(_mm_max_epi16); I2(_mm_max_epu8); I2(_mm_min_epi16); I2(_mm_min_epu8);
    I2(_mm_mulhi_epi16); I2(_mm_mulhi_epu16); I2(_mm_mullo_epi16); I2(_mm_mul_epu32); I2(_mm_sad_epu8);
    I2(_mm_and_si128); I2(_mm_andnot_si128); I2(_mm_or_si128); I2(_mm_xor_si128);
    I2(_mm_cmpeq_epi8); I2(_mm_cmpeq_epi16); I2(_mm_cmpeq_epi32);
    I2(_mm_cmpgt_epi8); I2(_mm_cmpgt_epi16); I2(_mm_cmpgt_epi32);
    I2(_mm_cmplt_epi8); I2(_mm_cmplt_epi16); I2(_mm_cmplt_epi32);
    I2(_mm_packs_epi16); I2(_mm_packs_epi32); I2(_mm_packus_epi16);
    I2(_mm_unpackhi_epi8); I2(_mm_unpackhi_epi16); I2(_mm_unpackhi_epi32); I2(_mm_unpackhi_epi64);
    I2(_mm_unpacklo_epi8); I2(_mm_unpacklo_epi16); I2(_mm_unpacklo_epi32); I2(_mm_unpacklo_epi64);
    for (c = 0; c < 10; c++) {
        __m128i cnt = _mm_cvtsi32_si128(counts[c]);
        int i;
        for (i = 0; i < NI; i++) {
            __m128i r;
            r = _mm_sll_epi16(vi[i], cnt); put_bytes("_mm_sll_epi16", i, counts[c], &r, 16);
            r = _mm_sll_epi32(vi[i], cnt); put_bytes("_mm_sll_epi32", i, counts[c], &r, 16);
            r = _mm_sll_epi64(vi[i], cnt); put_bytes("_mm_sll_epi64", i, counts[c], &r, 16);
            r = _mm_srl_epi16(vi[i], cnt); put_bytes("_mm_srl_epi16", i, counts[c], &r, 16);
            r = _mm_srl_epi32(vi[i], cnt); put_bytes("_mm_srl_epi32", i, counts[c], &r, 16);
            r = _mm_srl_epi64(vi[i], cnt); put_bytes("_mm_srl_epi64", i, counts[c], &r, 16);
            r = _mm_sra_epi16(vi[i], cnt); put_bytes("_mm_sra_epi16", i, counts[c], &r, 16);
            r = _mm_sra_epi32(vi[i], cnt); put_bytes("_mm_sra_epi32", i, counts[c], &r, 16);
        }
    }
    /* the immediate forms, each count a constant */
#define SHIFTS_AT(c) SH_I(_mm_slli_epi16, c); SH_I(_mm_slli_epi32, c); SH_I(_mm_slli_epi64, c); \
    SH_I(_mm_srli_epi16, c); SH_I(_mm_srli_epi32, c); SH_I(_mm_srli_epi64, c); \
    SH_I(_mm_srai_epi16, c); SH_I(_mm_srai_epi32, c)
    SHIFTS_AT(0); SHIFTS_AT(1); SHIFTS_AT(7); SHIFTS_AT(15); SHIFTS_AT(16);
    SHIFTS_AT(31); SHIFTS_AT(32); SHIFTS_AT(63); SHIFTS_AT(64); SHIFTS_AT(255);
#define BYTESHIFTS_AT(c) SH_CONST(_mm_slli_si128, c); SH_CONST(_mm_srli_si128, c)
    BYTESHIFTS_AT(0); BYTESHIFTS_AT(1); BYTESHIFTS_AT(4); BYTESHIFTS_AT(8); BYTESHIFTS_AT(15); BYTESHIFTS_AT(16);
    { int i; for (i = 0; i < NI; i++) put_int("_mm_movemask_epi8", i, 0, _mm_movemask_epi8(vi[i])); }
    { int i; for (i = 0; i < NI; i++) { __m128i r = _mm_move_epi64(vi[i]); put_bytes("_mm_move_epi64", i, 0, &r, 16); } }
#define SHUF_AT(c) SH_CONST(_mm_shuffle_epi32, c); SH_CONST(_mm_shufflehi_epi16, c); SH_CONST(_mm_shufflelo_epi16, c)
    SHUF_AT(0x00); SHUF_AT(0x1b); SHUF_AT(0xe4); SHUF_AT(0x4e); SHUF_AT(0xb1);
#define EXTRACT_AT(c) do { int i; for (i = 0; i < NI; i++) put_int("_mm_extract_epi16", i, c, _mm_extract_epi16(vi[i], c)); } while (0)
    EXTRACT_AT(0); EXTRACT_AT(1); EXTRACT_AT(2); EXTRACT_AT(3); EXTRACT_AT(4); EXTRACT_AT(5); EXTRACT_AT(6); EXTRACT_AT(7);
#define INSERT_AT(c) do { int i; for (i = 0; i < NI; i++) { __m128i r_ = _mm_insert_epi16(vi[i], -12345 - c, c); put_bytes("_mm_insert_epi16", i, c, &r_, 16); } } while (0)
    INSERT_AT(0); INSERT_AT(3); INSERT_AT(7);
}

static void casts(void)
{
    __m128 f = _mm_castsi128_ps(vi[4]);
    __m128d d = _mm_castsi128_pd(vi[4]);
    __m128i a = _mm_castps_si128(vf[1]), b = _mm_castpd_si128(vd[3]);
    __m128d c = _mm_castps_pd(vf[3]);
    __m128 e = _mm_castpd_ps(vd[2]);
    put_bytes("_mm_castsi128_ps", 0, 0, &f, 16); put_bytes("_mm_castsi128_pd", 0, 0, &d, 16);
    put_bytes("_mm_castps_si128", 0, 0, &a, 16); put_bytes("_mm_castpd_si128", 0, 0, &b, 16);
    put_bytes("_mm_castps_pd", 0, 0, &c, 16); put_bytes("_mm_castpd_ps", 0, 0, &e, 16);
}

/* the x64 / older names MSVC also has (they are aliases or the same
   operation, and must give the same bytes as their canonical forms) */
static void x64_names(void)
{
    static const float fs[4] = { 2.5f, -3.5f, 1.0e19f, -0.4f };
    static const long long lls[3] = { 0, -9007199254740993LL, 9223372036854775807LL };
    ALIGN16 __int64 nt[2];
    int k;
    for (k = 0; k < 4; k++) {
        __m128 a = _mm_set_ss(fs[k]);
        put_int("_mm_cvtss_si64x", k, 0, _mm_cvtss_si64x(a));
        put_int("_mm_cvttss_si64x", k, 0, _mm_cvttss_si64x(a));
    }
    for (k = 0; k < 3; k++) {
        __m128 r = _mm_cvtsi64x_ss(vf[2], lls[k]);
        put_bytes("_mm_cvtsi64x_ss", k, 0, &r, 16);
    }
    for (k = 0; k < NI; k++) {
        __m128i r = _mm_setl_epi64(vi[k]);
        put_bytes("_mm_setl_epi64", k, 0, &r, 16);
        nt[0] = 0x1111; nt[1] = 0x2222;
        _mm_stream_si64x(&nt[1], vi[k].m128i_i64[0]);
        put_bytes("_mm_stream_si64x", k, 0, nt, 16);
    }
}
int main(void)
{
    /* LF only, so the expected file does not depend on line-end conversion */
    _setmode(_fileno(stdout), _O_BINARY);
    init_inputs();
    sse_arith();
    sse_compare();
    sse_approx();
    sse_convert();
    sse_memory();
    sse_set_move();
    sse2_double();
    sse2_convert();
    sse2_set_memory();
    sse2_integer();
    casts();
    x64_names();
    printf("SIMD_VALUES_END\n");
    return 0;
}
