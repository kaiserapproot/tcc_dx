/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

/* xmmintrin.h for the tcc_dx include directory: SSE (__m128) intrinsics.

   TCC: the intrinsics are static __inline__ functions (or macros, where an
   operand must be an immediate) written with inline asm, in the MSVC spelling
   and with the MSVC types (layout and source compatible only: TCC passes a
   16-byte union by reference as MSVC does, but returns it through a hidden
   pointer where MSVC returns __m128 in xmm0, so a function returning __m128
   cannot be called between TCC- and MSVC-compiled code):
     - __m128 is a 16-byte aligned union with the MSVC member names
       (m128_f32, m128_i32, ...).  TCC has no vector types.
     - Operands are loaded with movups, so a by-value __m128 need not be
       aligned.  _mm_load_ps / _mm_store_ps / _mm_stream_ps use the aligned
       instructions and fault on a misaligned pointer, as with MSVC.
     - Only %xmm0-%xmm5 are used: xmm6 and up are callee-saved on Win64.
     - The barrier is the instruction itself; TCC accepts the "memory"
       clobber but does not act on it, and it does not move memory accesses
       across statements.
   _mm_sfence and _mm_pause come from psdk_inc/intrin-tcc.h (via _mingw.h).
   MMX (__m64 arithmetic) is not provided; __m64 exists only for the
   _mm_loadh_pi / _mm_loadl_pi / _mm_storeh_pi / _mm_storel_pi pointers.
   The values are checked against MSVC by dev\test\a9\simd_gate.bat.

   GCC / Clang: this directory can end up in front of the compiler's own
   headers, so pass the include on to the real <xmmintrin.h>. */

#ifndef __TINYC__
#include_next <xmmintrin.h>
#else

#ifndef _XMMINTRIN_H_INCLUDED
#define _XMMINTRIN_H_INCLUDED

#include <_mingw.h>

#if !defined(__x86_64__)
#error "xmmintrin.h: only x86-64 is supported by this TCC build"
#endif

/* code that tests these (and intrin.h, to skip its own prototypes) sees SSE */
#ifndef __SSE__
#define __SSE__ 1
#endif

#ifndef __TCC_M64_DEFINED
#define __TCC_M64_DEFINED
typedef union __attribute__((aligned(8))) __m64 {
  unsigned __int64 m64_u64;
  float m64_f32[2];
  char m64_i8[8];
  short m64_i16[4];
  int m64_i32[2];
  __int64 m64_i64;
  unsigned char m64_u8[8];
  unsigned short m64_u16[4];
  unsigned int m64_u32[2];
} __m64;
#endif

typedef union __attribute__((aligned(16))) __m128 {
  float m128_f32[4];
  unsigned __int64 m128_u64[2];
  char m128_i8[16];
  short m128_i16[8];
  int m128_i32[4];
  __int64 m128_i64[2];
  unsigned char m128_u8[16];
  unsigned short m128_u16[8];
  unsigned int m128_u32[4];
} __m128;

#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))

#define _MM_HINT_NTA 0
#define _MM_HINT_T0 1
#define _MM_HINT_T1 2
#define _MM_HINT_T2 3
#define _MM_HINT_ENTA 4

#define _MM_EXCEPT_MASK 0x003f
#define _MM_EXCEPT_INVALID 0x0001
#define _MM_EXCEPT_DENORM 0x0002
#define _MM_EXCEPT_DIV_ZERO 0x0004
#define _MM_EXCEPT_OVERFLOW 0x0008
#define _MM_EXCEPT_UNDERFLOW 0x0010
#define _MM_EXCEPT_INEXACT 0x0020

#define _MM_MASK_MASK 0x1f80
#define _MM_MASK_INVALID 0x0080
#define _MM_MASK_DENORM 0x0100
#define _MM_MASK_DIV_ZERO 0x0200
#define _MM_MASK_OVERFLOW 0x0400
#define _MM_MASK_UNDERFLOW 0x0800
#define _MM_MASK_INEXACT 0x1000

#define _MM_ROUND_MASK 0x6000
#define _MM_ROUND_NEAREST 0x0000
#define _MM_ROUND_DOWN 0x2000
#define _MM_ROUND_UP 0x4000
#define _MM_ROUND_TOWARD_ZERO 0x6000

#define _MM_FLUSH_ZERO_MASK 0x8000
#define _MM_FLUSH_ZERO_ON 0x8000
#define _MM_FLUSH_ZERO_OFF 0x0000

/* Building blocks.  %0 is the result's address, %1 / %2 the operands'.
   Every operand goes through movups into %xmm0 / %xmm1, and the result is
   stored from %xmm0 with movups. */
#define __TCC_XMM_LD1 "movups (%1), %%xmm0\n\t"
#define __TCC_XMM_LD2 "movups (%1), %%xmm0\n\tmovups (%2), %%xmm1\n\t"
#define __TCC_XMM_ST "\n\tmovups %%xmm0, (%0)"

/* r = a OP b, OP taking %xmm1 as source and %xmm0 as destination */
#define __TCC_XMM_BINOP(T, NAME, INSN) \
static __inline__ T NAME(T __a, T __b) \
{ \
  T __r; \
  __asm__ __volatile__(__TCC_XMM_LD2 INSN " %%xmm1, %%xmm0" __TCC_XMM_ST \
                       : : "r"(&__r), "r"(&__a), "r"(&__b) : "memory"); \
  return __r; \
}

/* the same with the operands swapped (b OP a): _mm_cmpgt_ps is cmplt on b, a */
#define __TCC_XMM_BINOP_SWAP(T, NAME, INSN) \
static __inline__ T NAME(T __a, T __b) \
{ \
  T __r; \
  __asm__ __volatile__(__TCC_XMM_LD2 INSN " %%xmm0, %%xmm1\n\tmovups %%xmm1, (%0)" \
                       : : "r"(&__r), "r"(&__a), "r"(&__b) : "memory"); \
  return __r; \
}

/* scalar compare with swapped operands: the low lane is b OP a, the upper
   lanes stay those of a (movss / movsd merges the low lane back into a) */
#define __TCC_XMM_CMPS_SWAP(T, NAME, INSN, MOV) \
static __inline__ T NAME(T __a, T __b) \
{ \
  T __r; \
  __asm__ __volatile__(__TCC_XMM_LD2 "movaps %%xmm1, %%xmm2\n\t" INSN " %%xmm0, %%xmm2\n\t" \
                       MOV " %%xmm2, %%xmm0" __TCC_XMM_ST \
                       : : "r"(&__r), "r"(&__a), "r"(&__b) : "memory"); \
  return __r; \
}

/* r = OP a */
#define __TCC_XMM_UNOP(T, NAME, INSN) \
static __inline__ T NAME(T __a) \
{ \
  T __r; \
  __asm__ __volatile__(__TCC_XMM_LD1 INSN " %%xmm0, %%xmm0" __TCC_XMM_ST \
                       : : "r"(&__r), "r"(&__a) : "memory"); \
  return __r; \
}

/* comiss / ucomiss and their SSE2 twins: the flags are stored by setcc and
   combined in C.  Unordered (a NaN) sets ZF, PF and CF; as with MSVC every
   predicate is then 0 except neq, which is 1. */
#define __TCC_XMM_COMI(T, NAME, INSN, EXPR) \
static __inline__ int NAME(T __a, T __b) \
{ \
  unsigned char __f[3]; \
  __asm__ __volatile__(__TCC_XMM_LD2 INSN " %%xmm1, %%xmm0\n\t" \
                       "setz (%0)\n\tsetc 1(%0)\n\tsetp 2(%0)" \
                       : : "r"(__f), "r"(&__a), "r"(&__b) : "memory"); \
  { int zf = __f[0], cf = __f[1], pf = __f[2]; (void)zf; (void)cf; (void)pf; return (EXPR); } \
}

/* arithmetic */
__TCC_XMM_BINOP(__m128, _mm_add_ss, "addss")
__TCC_XMM_BINOP(__m128, _mm_add_ps, "addps")
__TCC_XMM_BINOP(__m128, _mm_sub_ss, "subss")
__TCC_XMM_BINOP(__m128, _mm_sub_ps, "subps")
__TCC_XMM_BINOP(__m128, _mm_mul_ss, "mulss")
__TCC_XMM_BINOP(__m128, _mm_mul_ps, "mulps")
__TCC_XMM_BINOP(__m128, _mm_div_ss, "divss")
__TCC_XMM_BINOP(__m128, _mm_div_ps, "divps")
__TCC_XMM_UNOP(__m128, _mm_sqrt_ss, "sqrtss")
__TCC_XMM_UNOP(__m128, _mm_sqrt_ps, "sqrtps")
__TCC_XMM_UNOP(__m128, _mm_rcp_ss, "rcpss")
__TCC_XMM_UNOP(__m128, _mm_rcp_ps, "rcpps")
__TCC_XMM_UNOP(__m128, _mm_rsqrt_ss, "rsqrtss")
__TCC_XMM_UNOP(__m128, _mm_rsqrt_ps, "rsqrtps")
__TCC_XMM_BINOP(__m128, _mm_min_ss, "minss")
__TCC_XMM_BINOP(__m128, _mm_min_ps, "minps")
__TCC_XMM_BINOP(__m128, _mm_max_ss, "maxss")
__TCC_XMM_BINOP(__m128, _mm_max_ps, "maxps")

/* logical */
__TCC_XMM_BINOP(__m128, _mm_and_ps, "andps")
__TCC_XMM_BINOP(__m128, _mm_andnot_ps, "andnps")
__TCC_XMM_BINOP(__m128, _mm_or_ps, "orps")
__TCC_XMM_BINOP(__m128, _mm_xor_ps, "xorps")

/* compare: cmpps / cmpss predicates 0 eq, 1 lt, 2 le, 3 unord, 4 neq,
   5 nlt, 6 nle, 7 ord; gt / ge / ngt / nge swap the operands */
__TCC_XMM_BINOP(__m128, _mm_cmpeq_ss, "cmpss $0,")
__TCC_XMM_BINOP(__m128, _mm_cmpeq_ps, "cmpps $0,")
__TCC_XMM_BINOP(__m128, _mm_cmplt_ss, "cmpss $1,")
__TCC_XMM_BINOP(__m128, _mm_cmplt_ps, "cmpps $1,")
__TCC_XMM_BINOP(__m128, _mm_cmple_ss, "cmpss $2,")
__TCC_XMM_BINOP(__m128, _mm_cmple_ps, "cmpps $2,")
__TCC_XMM_CMPS_SWAP(__m128, _mm_cmpgt_ss, "cmpss $1,", "movss")
__TCC_XMM_BINOP_SWAP(__m128, _mm_cmpgt_ps, "cmpps $1,")
__TCC_XMM_CMPS_SWAP(__m128, _mm_cmpge_ss, "cmpss $2,", "movss")
__TCC_XMM_BINOP_SWAP(__m128, _mm_cmpge_ps, "cmpps $2,")
__TCC_XMM_BINOP(__m128, _mm_cmpneq_ss, "cmpss $4,")
__TCC_XMM_BINOP(__m128, _mm_cmpneq_ps, "cmpps $4,")
__TCC_XMM_BINOP(__m128, _mm_cmpnlt_ss, "cmpss $5,")
__TCC_XMM_BINOP(__m128, _mm_cmpnlt_ps, "cmpps $5,")
__TCC_XMM_BINOP(__m128, _mm_cmpnle_ss, "cmpss $6,")
__TCC_XMM_BINOP(__m128, _mm_cmpnle_ps, "cmpps $6,")
__TCC_XMM_CMPS_SWAP(__m128, _mm_cmpngt_ss, "cmpss $5,", "movss")
__TCC_XMM_BINOP_SWAP(__m128, _mm_cmpngt_ps, "cmpps $5,")
__TCC_XMM_CMPS_SWAP(__m128, _mm_cmpnge_ss, "cmpss $6,", "movss")
__TCC_XMM_BINOP_SWAP(__m128, _mm_cmpnge_ps, "cmpps $6,")
__TCC_XMM_BINOP(__m128, _mm_cmpord_ss, "cmpss $7,")
__TCC_XMM_BINOP(__m128, _mm_cmpord_ps, "cmpps $7,")
__TCC_XMM_BINOP(__m128, _mm_cmpunord_ss, "cmpss $3,")
__TCC_XMM_BINOP(__m128, _mm_cmpunord_ps, "cmpps $3,")

__TCC_XMM_COMI(__m128, _mm_comieq_ss, "comiss", zf && !pf)
__TCC_XMM_COMI(__m128, _mm_comilt_ss, "comiss", cf && !pf)
__TCC_XMM_COMI(__m128, _mm_comile_ss, "comiss", (cf || zf) && !pf)
__TCC_XMM_COMI(__m128, _mm_comigt_ss, "comiss", !cf && !zf)
__TCC_XMM_COMI(__m128, _mm_comige_ss, "comiss", !cf)
__TCC_XMM_COMI(__m128, _mm_comineq_ss, "comiss", !zf || pf)
__TCC_XMM_COMI(__m128, _mm_ucomieq_ss, "ucomiss", zf && !pf)
__TCC_XMM_COMI(__m128, _mm_ucomilt_ss, "ucomiss", cf && !pf)
__TCC_XMM_COMI(__m128, _mm_ucomile_ss, "ucomiss", (cf || zf) && !pf)
__TCC_XMM_COMI(__m128, _mm_ucomigt_ss, "ucomiss", !cf && !zf)
__TCC_XMM_COMI(__m128, _mm_ucomige_ss, "ucomiss", !cf)
__TCC_XMM_COMI(__m128, _mm_ucomineq_ss, "ucomiss", !zf || pf)

/* conversion (the rounding of cvt* follows MXCSR, as with MSVC) */
static __inline__ int _mm_cvtss_si32(__m128 __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtss2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ int _mm_cvttss_si32(__m128 __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvttss2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __int64 _mm_cvtss_si64(__m128 __a)
{
  __int64 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtss2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __int64 _mm_cvttss_si64(__m128 __a)
{
  __int64 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvttss2si %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

static __inline__ __m128 _mm_cvtsi32_ss(__m128 __a, int __b)
{
  __m128 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsi2ss %2, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__b) : "memory");
  return __r;
}

static __inline__ __m128 _mm_cvtsi64_ss(__m128 __a, __int64 __b)
{
  __m128 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "cvtsi2ss %2, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__b) : "memory");
  return __r;
}

static __inline__ float _mm_cvtss_f32(__m128 __a)
{
  return __a.m128_f32[0];
}

#define _mm_cvt_ss2si _mm_cvtss_si32
#define _mm_cvtt_ss2si _mm_cvttss_si32
#define _mm_cvt_si2ss _mm_cvtsi32_ss
/* the x64 names MSVC also has (intrin.h no longer declares them once
   __SSE__ is defined, so they must be here) */
#define _mm_cvtss_si64x _mm_cvtss_si64
#define _mm_cvttss_si64x _mm_cvttss_si64
#define _mm_cvtsi64x_ss _mm_cvtsi64_ss

/* set (plain data movement, so plain C) */
static __inline__ __m128 _mm_setzero_ps(void)
{
  __m128 __r;
  __r.m128_u64[0] = 0;
  __r.m128_u64[1] = 0;
  return __r;
}

static __inline__ __m128 _mm_undefined_ps(void)
{
  return _mm_setzero_ps();
}

static __inline__ __m128 _mm_set_ss(float __a)
{
  __m128 __r = _mm_setzero_ps();
  __r.m128_f32[0] = __a;
  return __r;
}

static __inline__ __m128 _mm_set1_ps(float __a)
{
  __m128 __r;
  __r.m128_f32[0] = __a;
  __r.m128_f32[1] = __a;
  __r.m128_f32[2] = __a;
  __r.m128_f32[3] = __a;
  return __r;
}

static __inline__ __m128 _mm_set_ps(float __e3, float __e2, float __e1, float __e0)
{
  __m128 __r;
  __r.m128_f32[0] = __e0;
  __r.m128_f32[1] = __e1;
  __r.m128_f32[2] = __e2;
  __r.m128_f32[3] = __e3;
  return __r;
}

static __inline__ __m128 _mm_setr_ps(float __e0, float __e1, float __e2, float __e3)
{
  return _mm_set_ps(__e3, __e2, __e1, __e0);
}

#define _mm_set_ps1 _mm_set1_ps

/* load */
static __inline__ __m128 _mm_load_ps(float const *__p)
{
  __m128 __r;
  __asm__ __volatile__("movaps (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_loadu_ps(float const *__p)
{
  __m128 __r;
  __asm__ __volatile__("movups (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_load_ss(float const *__p)
{
  __m128 __r;
  __asm__ __volatile__("movss (%1), %%xmm0" __TCC_XMM_ST : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_load1_ps(float const *__p)
{
  __m128 __r;
  __asm__ __volatile__("movss (%1), %%xmm0\n\tshufps $0, %%xmm0, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_loadr_ps(float const *__p)
{
  __m128 __r;
  __asm__ __volatile__("movaps (%1), %%xmm0\n\tshufps $0x1b, %%xmm0, %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_loadh_pi(__m128 __a, __m64 const *__p)
{
  __m128 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movhps (%2), %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__p) : "memory");
  return __r;
}

static __inline__ __m128 _mm_loadl_pi(__m128 __a, __m64 const *__p)
{
  __m128 __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movlps (%2), %%xmm0" __TCC_XMM_ST
                       : : "r"(&__r), "r"(&__a), "r"(__p) : "memory");
  return __r;
}

#define _mm_load_ps1 _mm_load1_ps

/* store */
static __inline__ void _mm_store_ps(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movaps %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storeu_ps(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movups %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_store_ss(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movss %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_store1_ps(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "shufps $0, %%xmm0, %%xmm0\n\tmovaps %%xmm0, (%0)"
                       : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storer_ps(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "shufps $0x1b, %%xmm0, %%xmm0\n\tmovaps %%xmm0, (%0)"
                       : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storeh_pi(__m64 *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movhps %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_storel_pi(__m64 *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movlps %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

static __inline__ void _mm_stream_ps(float *__p, __m128 __a)
{
  __asm__ __volatile__(__TCC_XMM_LD1 "movntps %%xmm0, (%0)" : : "r"(__p), "r"(&__a) : "memory");
}

#define _mm_store_ps1 _mm_store1_ps

/* move / shuffle / unpack */
__TCC_XMM_BINOP(__m128, _mm_move_ss, "movss")
__TCC_XMM_BINOP(__m128, _mm_movehl_ps, "movhlps")
__TCC_XMM_BINOP(__m128, _mm_movelh_ps, "movlhps")
__TCC_XMM_BINOP(__m128, _mm_unpackhi_ps, "unpckhps")
__TCC_XMM_BINOP(__m128, _mm_unpacklo_ps, "unpcklps")

static __inline__ int _mm_movemask_ps(__m128 __a)
{
  int __r;
  __asm__ __volatile__(__TCC_XMM_LD1 "movmskps %%xmm0, %0" : "=r"(__r) : "r"(&__a) : "memory");
  return __r;
}

/* the selector must be a constant, as with MSVC */
#define _mm_shuffle_ps(a, b, imm) __extension__ ({ \
  __m128 __tcc_a = (a), __tcc_b = (b), __tcc_r; \
  __asm__ __volatile__("movups (%1), %%xmm0\n\tmovups (%2), %%xmm1\n\tshufps %3, %%xmm1, %%xmm0\n\tmovups %%xmm0, (%0)" \
                       : : "r"(&__tcc_r), "r"(&__tcc_a), "r"(&__tcc_b), "i"((imm) & 0xff) : "memory"); \
  __tcc_r; })

/* control / status */
static __inline__ unsigned int _mm_getcsr(void)
{
  unsigned int __r;
  __asm__ __volatile__("stmxcsr (%0)" : : "r"(&__r) : "memory");
  return __r;
}

static __inline__ void _mm_setcsr(unsigned int __a)
{
  __asm__ __volatile__("ldmxcsr (%0)" : : "r"(&__a) : "memory");
}

#define _MM_GET_EXCEPTION_STATE() (_mm_getcsr() & _MM_EXCEPT_MASK)
#define _MM_SET_EXCEPTION_STATE(m) _mm_setcsr((_mm_getcsr() & ~_MM_EXCEPT_MASK) | (m))
#define _MM_GET_EXCEPTION_MASK() (_mm_getcsr() & _MM_MASK_MASK)
#define _MM_SET_EXCEPTION_MASK(m) _mm_setcsr((_mm_getcsr() & ~_MM_MASK_MASK) | (m))
#define _MM_GET_ROUNDING_MODE() (_mm_getcsr() & _MM_ROUND_MASK)
#define _MM_SET_ROUNDING_MODE(m) _mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | (m))
#define _MM_GET_FLUSH_ZERO_MODE() (_mm_getcsr() & _MM_FLUSH_ZERO_MASK)
#define _MM_SET_FLUSH_ZERO_MODE(m) _mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO_MASK) | (m))

/* prefetch: the hint must be one of the _MM_HINT_* constants */
static __inline__ void __tcc_prefetchnta(void const *__p) { __asm__ __volatile__("prefetchnta (%0)" : : "r"(__p)); }
static __inline__ void __tcc_prefetcht0(void const *__p) { __asm__ __volatile__("prefetcht0 (%0)" : : "r"(__p)); }
static __inline__ void __tcc_prefetcht1(void const *__p) { __asm__ __volatile__("prefetcht1 (%0)" : : "r"(__p)); }
static __inline__ void __tcc_prefetcht2(void const *__p) { __asm__ __volatile__("prefetcht2 (%0)" : : "r"(__p)); }
#define _mm_prefetch(p, hint) \
  ((hint) == _MM_HINT_T0 ? __tcc_prefetcht0((void const *)(p)) : \
   (hint) == _MM_HINT_T1 ? __tcc_prefetcht1((void const *)(p)) : \
   (hint) == _MM_HINT_T2 ? __tcc_prefetcht2((void const *)(p)) : \
   __tcc_prefetchnta((void const *)(p)))

#define _MM_TRANSPOSE4_PS(row0, row1, row2, row3) do { \
  __m128 __tcc_t0 = _mm_unpacklo_ps((row0), (row1)); \
  __m128 __tcc_t1 = _mm_unpacklo_ps((row2), (row3)); \
  __m128 __tcc_t2 = _mm_unpackhi_ps((row0), (row1)); \
  __m128 __tcc_t3 = _mm_unpackhi_ps((row2), (row3)); \
  (row0) = _mm_movelh_ps(__tcc_t0, __tcc_t1); \
  (row1) = _mm_movehl_ps(__tcc_t1, __tcc_t0); \
  (row2) = _mm_movelh_ps(__tcc_t2, __tcc_t3); \
  (row3) = _mm_movehl_ps(__tcc_t3, __tcc_t2); \
} while (0)

#endif /* _XMMINTRIN_H_INCLUDED */
#endif /* __TINYC__ */
