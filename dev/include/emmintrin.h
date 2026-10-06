/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

/* emmintrin.h for the tcc_dx include directory: SSE2 (__m128d, __m128i).

   TCC: the same scheme as xmmintrin.h (static __inline__ asm, MSVC types and
   member names, movups loads, %xmm0-%xmm5 only, macros where an operand must
   be an immediate).  _mm_lfence, _mm_mfence and _mm_pause come from
   psdk_inc/intrin-tcc.h (via _mingw.h).  The __m64 forms (_mm_add_si64,
   _mm_movepi64_pi64, ...) are not provided.
   The shift counts of _mm_slli_* / _mm_srli_* / _mm_srai_* need not be
   constants: they go through a register, and a count past the element width
   gives 0 (or the sign) exactly as the immediate form does.
   The values are checked against MSVC by dev\test\a9\simd_gate.bat.

   GCC / Clang: pass the include on to the real <emmintrin.h>. */

#ifndef __TINYC__
#include_next <emmintrin.h>
#else

#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED

#include <xmmintrin.h>

#ifndef __SSE2__
#define __SSE2__ 1
#endif

typedef union __attribute__((aligned(16))) __m128d {
  double m128d_f64[2];
} __m128d;

typedef union __attribute__((aligned(16))) __m128i {
  char m128i_i8[16];
  short m128i_i16[8];
  int m128i_i32[4];
  __int64 m128i_i64[2];
  unsigned char m128i_u8[16];
  unsigned short m128i_u16[8];
  unsigned int m128i_u32[4];
  unsigned __int64 m128i_u64[2];
} __m128i;

/* double: arithmetic */
__TCC_XMM_BINOP(__m128d, _mm_add_sd, "addsd")
__TCC_XMM_BINOP(__m128d, _mm_add_pd, "addpd")
__TCC_XMM_BINOP(__m128d, _mm_sub_sd, "subsd")
__TCC_XMM_BINOP(__m128d, _mm_sub_pd, "subpd")
__TCC_XMM_BINOP(__m128d, _mm_mul_sd, "mulsd")
__TCC_XMM_BINOP(__m128d, _mm_mul_pd, "mulpd")
__TCC_XMM_BINOP(__m128d, _mm_div_sd, "divsd")
__TCC_XMM_BINOP(__m128d, _mm_div_pd, "divpd")
/* low = sqrt(b0), high = a1 */
__TCC_XMM_BINOP(__m128d, _mm_sqrt_sd, "sqrtsd")
__TCC_XMM_UNOP(__m128d, _mm_sqrt_pd, "sqrtpd")
__TCC_XMM_BINOP(__m128d, _mm_min_sd, "minsd")
__TCC_XMM_BINOP(__m128d, _mm_min_pd, "minpd")
__TCC_XMM_BINOP(__m128d, _mm_max_sd, "maxsd")
__TCC_XMM_BINOP(__m128d, _mm_max_pd, "maxpd")

/* double: logical */
__TCC_XMM_BINOP(__m128d, _mm_and_pd, "andpd")
__TCC_XMM_BINOP(__m128d, _mm_andnot_pd, "andnpd")
__TCC_XMM_BINOP(__m128d, _mm_or_pd, "orpd")
__TCC_XMM_BINOP(__m128d, _mm_xor_pd, "xorpd")

/* double: compare (the same predicates as cmpps) */
__TCC_XMM_BINOP(__m128d, _mm_cmpeq_sd, "cmpsd $0,")
__TCC_XMM_BINOP(__m128d, _mm_cmpeq_pd, "cmppd $0,")
__TCC_XMM_BINOP(__m128d, _mm_cmplt_sd, "cmpsd $1,")
__TCC_XMM_BINOP(__m128d, _mm_cmplt_pd, "cmppd $1,")
__TCC_XMM_BINOP(__m128d, _mm_cmple_sd, "cmpsd $2,")
__TCC_XMM_BINOP(__m128d, _mm_cmple_pd, "cmppd $2,")
__TCC_XMM_CMPS_SWAP(__m128d, _mm_cmpgt_sd, "cmpsd $1,", "movsd")
__TCC_XMM_BINOP_SWAP(__m128d, _mm_cmpgt_pd, "cmppd $1,")
__TCC_XMM_CMPS_SWAP(__m128d, _mm_cmpge_sd, "cmpsd $2,", "movsd")
__TCC_XMM_BINOP_SWAP(__m128d, _mm_cmpge_pd, "cmppd $2,")
__TCC_XMM_BINOP(__m128d, _mm_cmpneq_sd, "cmpsd $4,")
__TCC_XMM_BINOP(__m128d, _mm_cmpneq_pd, "cmppd $4,")
__TCC_XMM_BINOP(__m128d, _mm_cmpnlt_sd, "cmpsd $5,")
__TCC_XMM_BINOP(__m128d, _mm_cmpnlt_pd, "cmppd $5,")
__TCC_XMM_BINOP(__m128d, _mm_cmpnle_sd, "cmpsd $6,")
__TCC_XMM_BINOP(__m128d, _mm_cmpnle_pd, "cmppd $6,")
__TCC_XMM_CMPS_SWAP(__m128d, _mm_cmpngt_sd, "cmpsd $5,", "movsd")
__TCC_XMM_BINOP_SWAP(__m128d, _mm_cmpngt_pd, "cmppd $5,")
__TCC_XMM_CMPS_SWAP(__m128d, _mm_cmpnge_sd, "cmpsd $6,", "movsd")
__TCC_XMM_BINOP_SWAP(__m128d, _mm_cmpnge_pd, "cmppd $6,")
__TCC_XMM_BINOP(__m128d, _mm_cmpord_sd, "cmpsd $7,")
__TCC_XMM_BINOP(__m128d, _mm_cmpord_pd, "cmppd $7,")
__TCC_XMM_BINOP(__m128d, _mm_cmpunord_sd, "cmpsd $3,")
__TCC_XMM_BINOP(__m128d, _mm_cmpunord_pd, "cmppd $3,")

__TCC_XMM_COMI(__m128d, _mm_comieq_sd, "comisd", zf && !pf)
__TCC_XMM_COMI(__m128d, _mm_comilt_sd, "comisd", cf && !pf)
__TCC_XMM_COMI(__m128d, _mm_comile_sd, "comisd", (cf || zf) && !pf)
__TCC_XMM_COMI(__m128d, _mm_comigt_sd, "comisd", !cf && !zf)
__TCC_XMM_COMI(__m128d, _mm_comige_sd, "comisd", !cf)
__TCC_XMM_COMI(__m128d, _mm_comineq_sd, "comisd", !zf || pf)
__TCC_XMM_COMI(__m128d, _mm_ucomieq_sd, "ucomisd", zf && !pf)
__TCC_XMM_COMI(__m128d, _mm_ucomilt_sd, "ucomisd", cf && !pf)
__TCC_XMM_COMI(__m128d, _mm_ucomile_sd, "ucomisd", (cf || zf) && !pf)
__TCC_XMM_COMI(__m128d, _mm_ucomigt_sd, "ucomisd", !cf && !zf)
__TCC_XMM_COMI(__m128d, _mm_ucomige_sd, "ucomisd", !cf)
__TCC_XMM_COMI(__m128d, _mm_ucomineq_sd, "ucomisd", !zf || pf)

/* conversion between vector types (rounding follows MXCSR) */
#define __TCC_XMM_CVT(TR, TA, NAME, INSN) \
static __inline__ TR NAME(TA __a) \
{ \
  TR __r; \
  __asm__ __volatile__(__TCC_XMM_LD1 INSN " %%xmm0, %%xmm0" __TCC_XMM_ST \
                       : : "r"(&__r), "r"(&__a) : "memory"); \
  return __r; \
}
__TCC_XMM_CVT(__m128d, __m128i, _mm_cvtepi32_pd, "cvtdq2pd")
__TCC_XMM_CVT(__m128, __m128i, _mm_cvtepi32_ps, "cvtdq2ps")
__TCC_XMM_CVT(__m128i, __m128d, _mm_cvtpd_epi32, "cvtpd2dq")
__TCC_XMM_CVT(__m128, __m128d, _mm_cvtpd_ps, "cvtpd2ps")
__TCC_XMM_CVT(__m128i, __m128, _mm_cvtps_epi32, "cvtps2dq")
__TCC_XMM_CVT(__m128d, __m128, _mm_cvtps_pd, "cvtps2pd")
__TCC_XMM_CVT(__m128i, __m128d, _mm_cvttpd_epi32, "cvttpd2dq")
__TCC_XMM_CVT(__m128i, __m128, _mm_cvttps_epi32, "cvttps2dq")

/* low = (float)b0, upper three lanes of a */
static __inline__ __m128 _mm_cvtsd_ss(__m128 __a, __m128d __b)
{
  __m128 __r;
  __asm__ __volatile__(__TCC_XMM_LD2 "cvtsd2ss %%xmm1, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(&__b) : "memory");
  return __r;
}

/* low = (double)b0, high lane of a */
static __inline__ __m128d _mm_cvtss_sd(__m128d __a, __m128 __b)
{
  __m128d __r;
  __asm__ __volatile__(__TCC_XMM_LD2 "cvtss2sd %%xmm1, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(&__b) : "memory");
  return __r;
}

/* conversion to and from scalars */
static __inline__ int _mm_cvtsd_si32(__m128d __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsd2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ int _mm_cvttsd_si32(__m128d __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvttsd2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __int64 _mm_cvtsd_si64(__m128d __a)
{
  __int64 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsd2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __int64 _mm_cvttsd_si64(__m128d __a)
{
  __int64 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvttsd2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __m128d _mm_cvtsi32_sd(__m128d __a, int __b)
{
  __m128d __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsi2sd %2, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__b) : "memory");
  return __r;
}

static __inline__ __m128d _mm_cvtsi64_sd(__m128d __a, __int64 __b)
{
  __m128d __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsi2sd %2, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__b) : "memory");
  return __r;
}

static __inline__ double _mm_cvtsd_f64(__m128d __a)
{
  return __a.m128d_f64[0];
}

static __inline__ int _mm_cvtsi128_si32(__m128i __a)
{
  return __a.m128i_i32[0];
}

static __inline__ __int64 _mm_cvtsi128_si64(__m128i __a)
{
  return __a.m128i_i64[0];
}

static __inline__ __m128i _mm_cvtsi32_si128(int __a)
{
  __m128i __r;
  __r.m128i_i32[0] = __a;
  __r.m128i_i32[1] = 0;
  __r.m128i_i64[1] = 0;
  return __r;
}

static __inline__ __m128i _mm_cvtsi64_si128(__int64 __a)
{
  __m128i __r;
  __r.m128i_i64[0] = __a;
  __r.m128i_i64[1] = 0;
  return __r;
}

#define _mm_cvtsd_si64x _mm_cvtsd_si64
#define _mm_cvttsd_si64x _mm_cvttsd_si64
#define _mm_cvtsi64x_sd _mm_cvtsi64_sd
#define _mm_cvtsi128_si64x _mm_cvtsi128_si64
#define _mm_cvtsi64x_si128 _mm_cvtsi64_si128

/* double: set (plain C) */
static __inline__ __m128d _mm_setzero_pd(void)
{
  __m128d __r;
  __r.m128d_f64[0] = 0.0;
  __r.m128d_f64[1] = 0.0;
  return __r;
}

static __inline__ __m128d _mm_undefined_pd(void)
{
  return _mm_setzero_pd();
}

static __inline__ __m128d _mm_set_sd(double __a)
{
  __m128d __r;
  __r.m128d_f64[0] = __a;
  __r.m128d_f64[1] = 0.0;
  return __r;
}

static __inline__ __m128d _mm_set1_pd(double __a)
{
  __m128d __r;
  __r.m128d_f64[0] = __a;
  __r.m128d_f64[1] = __a;
  return __r;
}

static __inline__ __m128d _mm_set_pd(double __e1, double __e0)
{
  __m128d __r;
  __r.m128d_f64[0] = __e0;
  __r.m128d_f64[1] = __e1;
  return __r;
}

static __inline__ __m128d _mm_setr_pd(double __e0, double __e1)
{
  return _mm_set_pd(__e1, __e0);
}

#define _mm_set_pd1 _mm_set1_pd

/* integer: set (plain C) */
static __inline__ __m128i _mm_setzero_si128(void)
{
  __m128i __r;
  __r.m128i_u64[0] = 0;
  __r.m128i_u64[1] = 0;
  return __r;
}

static __inline__ __m128i _mm_undefined_si128(void)
{
  return _mm_setzero_si128();
}

static __inline__ __m128i _mm_set_epi64x(__int64 __e1, __int64 __e0)
{
  __m128i __r;
  __r.m128i_i64[0] = __e0;
  __r.m128i_i64[1] = __e1;
  return __r;
}

static __inline__ __m128i _mm_set_epi32(int __e3, int __e2, int __e1, int __e0)
{
  __m128i __r;
  __r.m128i_i32[0] = __e0;
  __r.m128i_i32[1] = __e1;
  __r.m128i_i32[2] = __e2;
  __r.m128i_i32[3] = __e3;
  return __r;
}

static __inline__ __m128i _mm_set_epi16(short __e7, short __e6, short __e5, short __e4,
                                        short __e3, short __e2, short __e1, short __e0)
{
  __m128i __r;
  __r.m128i_i16[0] = __e0;
  __r.m128i_i16[1] = __e1;
  __r.m128i_i16[2] = __e2;
  __r.m128i_i16[3] = __e3;
  __r.m128i_i16[4] = __e4;
  __r.m128i_i16[5] = __e5;
  __r.m128i_i16[6] = __e6;
  __r.m128i_i16[7] = __e7;
  return __r;
}

static __inline__ __m128i _mm_set_epi8(char __e15, char __e14, char __e13, char __e12,
                                       char __e11, char __e10, char __e9, char __e8,
                                       char __e7, char __e6, char __e5, char __e4,
                                       char __e3, char __e2, char __e1, char __e0)
{
  __m128i __r;
  __r.m128i_i8[0] = __e0;
  __r.m128i_i8[1] = __e1;
  __r.m128i_i8[2] = __e2;
  __r.m128i_i8[3] = __e3;
  __r.m128i_i8[4] = __e4;
  __r.m128i_i8[5] = __e5;
  __r.m128i_i8[6] = __e6;
  __r.m128i_i8[7] = __e7;
  __r.m128i_i8[8] = __e8;
  __r.m128i_i8[9] = __e9;
  __r.m128i_i8[10] = __e10;
  __r.m128i_i8[11] = __e11;
  __r.m128i_i8[12] = __e12;
  __r.m128i_i8[13] = __e13;
  __r.m128i_i8[14] = __e14;
  __r.m128i_i8[15] = __e15;
  return __r;
}

static __inline__ __m128i _mm_set1_epi64x(__int64 __a)
{
  return _mm_set_epi64x(__a, __a);
}

static __inline__ __m128i _mm_set1_epi32(int __a)
{
  return _mm_set_epi32(__a, __a, __a, __a);
}

static __inline__ __m128i _mm_set1_epi16(short __a)
{
  return _mm_set_epi16(__a, __a, __a, __a, __a, __a, __a, __a);
}

static __inline__ __m128i _mm_set1_epi8(char __a)
{
  return _mm_set_epi8(__a, __a, __a, __a, __a, __a, __a, __a,
                      __a, __a, __a, __a, __a, __a, __a, __a);
}

static __inline__ __m128i _mm_setr_epi32(int __e0, int __e1, int __e2, int __e3)
{
  return _mm_set_epi32(__e3, __e2, __e1, __e0);
}

static __inline__ __m128i _mm_setr_epi16(short __e0, short __e1, short __e2, short __e3,
                                         short __e4, short __e5, short __e6, short __e7)
{
  return _mm_set_epi16(__e7, __e6, __e5, __e4, __e3, __e2, __e1, __e0);
}

static __inline__ __m128i _mm_setr_epi8(char __e0, char __e1, char __e2, char __e3,
                                        char __e4, char __e5, char __e6, char __e7,
                                        char __e8, char __e9, char __e10, char __e11,
                                        char __e12, char __e13, char __e14, char __e15)
{
  return _mm_set_epi8(__e15, __e14, __e13, __e12, __e11, __e10, __e9, __e8,
                      __e7, __e6, __e5, __e4, __e3, __e2, __e1, __e0);
}

/* double: load / store */
static __inline__ __m128d _mm_load_pd(double const *__p)
{
  __m128d __r;
  __asm__ __volatile__("movapd (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_loadu_pd(double const *__p)
{
  __m128d __r;
  __asm__ __volatile__("movupd (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_load_sd(double const *__p)
{
  __m128d __r;
  __asm__ __volatile__("movsd (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_load1_pd(double const *__p)
{
  __m128d __r;
  __asm__ __volatile__("movsd (%1), %%xmm0\n\tunpcklpd %%xmm0, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_loadr_pd(double const *__p)
{
  __m128d __r;
  __asm__ __volatile__("movapd (%1), %%xmm0\n\tshufpd $1, %%xmm0, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_loadh_pd(__m128d __a, double const *__p)
{
  __m128d __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movhpd (%2), %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128d _mm_loadl_pd(__m128d __a, double const *__p)
{
  __m128d __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movlpd (%2), %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__p) : "memory");
  return __r;
}

#define _mm_load_pd1 _mm_load1_pd

static __inline__ void _mm_store_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movapd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storeu_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movupd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_store_sd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movsd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_store1_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "unpcklpd %%xmm0, %%xmm0\n\tmovapd %%xmm0, (%0)"
                       : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storer_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "shufpd $1, %%xmm0, %%xmm0\n\tmovapd %%xmm0, (%0)"
                       : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storeh_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movhpd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storel_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movlpd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_stream_pd(double *__p, __m128d __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movntpd %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

#define _mm_store_pd1 _mm_store1_pd

/* integer: load / store */
static __inline__ __m128i _mm_load_si128(__m128i const *__p)
{
  __m128i __r;
  __asm__ __volatile__("movdqa (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128i _mm_loadu_si128(__m128i const *__p)
{
  __m128i __r;
  __asm__ __volatile__("movdqu (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

/* the low 8 bytes, upper half zero */
static __inline__ __m128i _mm_loadl_epi64(__m128i const *__p)
{
  __m128i __r;
  __asm__ __volatile__("movq (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ void _mm_store_si128(__m128i *__p, __m128i __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movdqa %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storeu_si128(__m128i *__p, __m128i __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movdqu %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storel_epi64(__m128i *__p, __m128i __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movq %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_stream_si128(__m128i *__p, __m128i __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movntdq %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_stream_si32(int *__p, int __a)
{
  __asm__ __volatile__("movnti %1, (%0)" : : "r"(__p), "r"(__a) : "memory");
}

static __inline__ void _mm_stream_si64(__int64 *__p, __int64 __a)
{
  __asm__ __volatile__("movnti %1, (%0)" : : "r"(__p), "r"(__a) : "memory");
}

/* bytes of a whose mask byte has the top bit set are stored to p */
static __inline__ void _mm_maskmoveu_si128(__m128i __a, __m128i __mask, char *__p)
{
  __asm__ __volatile__("movups (%0), %%xmm0\n\tmovups (%1), %%xmm1\n\tmaskmovdqu %%xmm1, %%xmm0"
                       : : "r"(&__a), "r"(&__mask), "D"(__p) : "memory");
}

/* integer: arithmetic */
__TCC_XMM_BINOP(__m128i, _mm_add_epi8, "paddb")
__TCC_XMM_BINOP(__m128i, _mm_add_epi16, "paddw")
__TCC_XMM_BINOP(__m128i, _mm_add_epi32, "paddd")
__TCC_XMM_BINOP(__m128i, _mm_add_epi64, "paddq")
__TCC_XMM_BINOP(__m128i, _mm_adds_epi8, "paddsb")
__TCC_XMM_BINOP(__m128i, _mm_adds_epi16, "paddsw")
__TCC_XMM_BINOP(__m128i, _mm_adds_epu8, "paddusb")
__TCC_XMM_BINOP(__m128i, _mm_adds_epu16, "paddusw")
__TCC_XMM_BINOP(__m128i, _mm_sub_epi8, "psubb")
__TCC_XMM_BINOP(__m128i, _mm_sub_epi16, "psubw")
__TCC_XMM_BINOP(__m128i, _mm_sub_epi32, "psubd")
__TCC_XMM_BINOP(__m128i, _mm_sub_epi64, "psubq")
__TCC_XMM_BINOP(__m128i, _mm_subs_epi8, "psubsb")
__TCC_XMM_BINOP(__m128i, _mm_subs_epi16, "psubsw")
__TCC_XMM_BINOP(__m128i, _mm_subs_epu8, "psubusb")
__TCC_XMM_BINOP(__m128i, _mm_subs_epu16, "psubusw")
__TCC_XMM_BINOP(__m128i, _mm_avg_epu8, "pavgb")
__TCC_XMM_BINOP(__m128i, _mm_avg_epu16, "pavgw")
__TCC_XMM_BINOP(__m128i, _mm_madd_epi16, "pmaddwd")
__TCC_XMM_BINOP(__m128i, _mm_max_epi16, "pmaxsw")
__TCC_XMM_BINOP(__m128i, _mm_max_epu8, "pmaxub")
__TCC_XMM_BINOP(__m128i, _mm_min_epi16, "pminsw")
__TCC_XMM_BINOP(__m128i, _mm_min_epu8, "pminub")
__TCC_XMM_BINOP(__m128i, _mm_mulhi_epi16, "pmulhw")
__TCC_XMM_BINOP(__m128i, _mm_mulhi_epu16, "pmulhuw")
__TCC_XMM_BINOP(__m128i, _mm_mullo_epi16, "pmullw")
__TCC_XMM_BINOP(__m128i, _mm_mul_epu32, "pmuludq")
__TCC_XMM_BINOP(__m128i, _mm_sad_epu8, "psadbw")

/* integer: logical */
__TCC_XMM_BINOP(__m128i, _mm_and_si128, "pand")
__TCC_XMM_BINOP(__m128i, _mm_andnot_si128, "pandn")
__TCC_XMM_BINOP(__m128i, _mm_or_si128, "por")
__TCC_XMM_BINOP(__m128i, _mm_xor_si128, "pxor")

/* integer: shifts by the count in the low 64 bits of count */
__TCC_XMM_BINOP(__m128i, _mm_sll_epi16, "psllw")
__TCC_XMM_BINOP(__m128i, _mm_sll_epi32, "pslld")
__TCC_XMM_BINOP(__m128i, _mm_sll_epi64, "psllq")
__TCC_XMM_BINOP(__m128i, _mm_srl_epi16, "psrlw")
__TCC_XMM_BINOP(__m128i, _mm_srl_epi32, "psrld")
__TCC_XMM_BINOP(__m128i, _mm_srl_epi64, "psrlq")
__TCC_XMM_BINOP(__m128i, _mm_sra_epi16, "psraw")
__TCC_XMM_BINOP(__m128i, _mm_sra_epi32, "psrad")

/* integer: shifts by an int count, as the 8-bit immediate of the
   instruction would take it */
#define __TCC_XMM_SHIFTI(NAME, INSN) \
static __inline__ __m128i NAME(__m128i __a, int __imm) \
{ \
  __m128i __r; \
  unsigned int __n = (unsigned int)__imm & 0xff; \
  __asm__ __volatile__(__TCC_XMM_LD1 "movd %2, %%xmm1\n\t" INSN " %%xmm1, %%xmm0" __TCC_XMM_ST \
                       : : "r"(&__r), "r"(&__a), "r"(__n) : "memory"); \
  return __r; \
}
__TCC_XMM_SHIFTI(_mm_slli_epi16, "psllw")
__TCC_XMM_SHIFTI(_mm_slli_epi32, "pslld")
__TCC_XMM_SHIFTI(_mm_slli_epi64, "psllq")
__TCC_XMM_SHIFTI(_mm_srli_epi16, "psrlw")
__TCC_XMM_SHIFTI(_mm_srli_epi32, "psrld")
__TCC_XMM_SHIFTI(_mm_srli_epi64, "psrlq")
__TCC_XMM_SHIFTI(_mm_srai_epi16, "psraw")
__TCC_XMM_SHIFTI(_mm_srai_epi32, "psrad")

/* whole-register byte shifts: the count must be a constant */
#define _mm_slli_si128(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpslldq %2, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })
#define _mm_srli_si128(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpsrldq %2, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })
#define _mm_bslli_si128 _mm_slli_si128
#define _mm_bsrli_si128 _mm_srli_si128

/* integer: compare (lt swaps the operands of gt) */
__TCC_XMM_BINOP(__m128i, _mm_cmpeq_epi8, "pcmpeqb")
__TCC_XMM_BINOP(__m128i, _mm_cmpeq_epi16, "pcmpeqw")
__TCC_XMM_BINOP(__m128i, _mm_cmpeq_epi32, "pcmpeqd")
__TCC_XMM_BINOP(__m128i, _mm_cmpgt_epi8, "pcmpgtb")
__TCC_XMM_BINOP(__m128i, _mm_cmpgt_epi16, "pcmpgtw")
__TCC_XMM_BINOP(__m128i, _mm_cmpgt_epi32, "pcmpgtd")
__TCC_XMM_BINOP_SWAP(__m128i, _mm_cmplt_epi8, "pcmpgtb")
__TCC_XMM_BINOP_SWAP(__m128i, _mm_cmplt_epi16, "pcmpgtw")
__TCC_XMM_BINOP_SWAP(__m128i, _mm_cmplt_epi32, "pcmpgtd")

/* pack / unpack */
__TCC_XMM_BINOP(__m128i, _mm_packs_epi16, "packsswb")
__TCC_XMM_BINOP(__m128i, _mm_packs_epi32, "packssdw")
__TCC_XMM_BINOP(__m128i, _mm_packus_epi16, "packuswb")
__TCC_XMM_BINOP(__m128i, _mm_unpackhi_epi8, "punpckhbw")
__TCC_XMM_BINOP(__m128i, _mm_unpackhi_epi16, "punpckhwd")
__TCC_XMM_BINOP(__m128i, _mm_unpackhi_epi32, "punpckhdq")
__TCC_XMM_BINOP(__m128i, _mm_unpackhi_epi64, "punpckhqdq")
__TCC_XMM_BINOP(__m128i, _mm_unpacklo_epi8, "punpcklbw")
__TCC_XMM_BINOP(__m128i, _mm_unpacklo_epi16, "punpcklwd")
__TCC_XMM_BINOP(__m128i, _mm_unpacklo_epi32, "punpckldq")
__TCC_XMM_BINOP(__m128i, _mm_unpacklo_epi64, "punpcklqdq")
__TCC_XMM_BINOP(__m128d, _mm_unpackhi_pd, "unpckhpd")
__TCC_XMM_BINOP(__m128d, _mm_unpacklo_pd, "unpcklpd")

/* move / mask */
__TCC_XMM_BINOP(__m128d, _mm_move_sd, "movsd")
/* the low 64 bits, upper half zero */
__TCC_XMM_UNOP(__m128i, _mm_move_epi64, "movq")

static __inline__ int _mm_movemask_epi8(__m128i __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "pmovmskb %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ int _mm_movemask_pd(__m128d __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movmskpd %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

/* shuffle / extract / insert: the selector must be a constant */
#define _mm_shuffle_epi32(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpshufd %2, %%xmm0, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })
#define _mm_shufflehi_epi16(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpshufhw %2, %%xmm0, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })
#define _mm_shufflelo_epi16(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpshuflw %2, %%xmm0, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })
#define _mm_shuffle_pd(a, b, imm) __extension__ ({ \
  __m128d __tcc_a = (a), __tcc_b = (b), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tmovups (%2), %%xmm1\n\tshufpd %3, %%xmm1, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "r"(&__tcc_b), "i"((imm) & 3) : "memory"); \
  __tcc_r; })
#define _mm_extract_epi16(a, imm) __extension__ ({ \
  __m128i __tcc_a = (a); \
  int __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpextrw %2, %%xmm0, %0" \
                       : "=r"(__tcc_r) : "r"(&__tcc_a), "i"((imm) & 7) : "memory"); \
  __tcc_r; })
#define _mm_insert_epi16(a, b, imm) __extension__ ({ \
  __m128i __tcc_a = (a), __tcc_r; \
  int __tcc_b = (b); \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tpinsrw %3, %2, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "r"(__tcc_b), "i"((imm) & 7) : "memory"); \
  __tcc_r; })

/* casts: the same 16 bytes seen as another type */
static __inline__ __m128 _mm_castpd_ps(__m128d __a) { __m128 __r; __r.m128_u64[0] = ((__m128i *)&__a)->m128i_u64[0]; __r.m128_u64[1] = ((__m128i *)&__a)->m128i_u64[1]; return __r; }
static __inline__ __m128i _mm_castpd_si128(__m128d __a) { __m128i __r; __r.m128i_u64[0] = ((__m128i *)&__a)->m128i_u64[0]; __r.m128i_u64[1] = ((__m128i *)&__a)->m128i_u64[1]; return __r; }
static __inline__ __m128d _mm_castps_pd(__m128 __a) { __m128d __r; ((__m128i *)&__r)->m128i_u64[0] = __a.m128_u64[0]; ((__m128i *)&__r)->m128i_u64[1] = __a.m128_u64[1]; return __r; }
static __inline__ __m128i _mm_castps_si128(__m128 __a) { __m128i __r; __r.m128i_u64[0] = __a.m128_u64[0]; __r.m128i_u64[1] = __a.m128_u64[1]; return __r; }
static __inline__ __m128d _mm_castsi128_pd(__m128i __a) { __m128d __r; ((__m128i *)&__r)->m128i_u64[0] = __a.m128i_u64[0]; ((__m128i *)&__r)->m128i_u64[1] = __a.m128i_u64[1]; return __r; }
static __inline__ __m128 _mm_castsi128_ps(__m128i __a) { __m128 __r; __r.m128_u64[0] = __a.m128i_u64[0]; __r.m128_u64[1] = __a.m128i_u64[1]; return __r; }

/* cache */
static __inline__ void _mm_clflush(void const *__p)
{
  __asm__ __volatile__("clflush (%0)" : : "r"(__p) : "memory");
}

#endif /* _EMMINTRIN_H_INCLUDED */
#endif /* __TINYC__ */
