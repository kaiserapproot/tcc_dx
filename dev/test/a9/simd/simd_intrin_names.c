/* The x64 / older intrinsic names reached the way MSVC code reaches them,
   through <intrin.h> alone: _mm_cvtss_si64x, _mm_cvttss_si64x,
   _mm_cvtsi64x_ss, _mm_stream_si64x and _mm_setl_epi64.  The SSE / SSE2
   gate otherwise calls the names the two headers define, so a name that
   should exist but is missing from them would go unnoticed (intrin.h stops
   declaring its own SSE prototypes once emmintrin.h defines __SSE2__).
   Built as C here and as C++ by simd_intrin_names_cpp.cpp. */
#include <windows.h>
#include <intrin.h>

int main(void)
{
    __m128 a = _mm_set_ss(-2.5f);
    __m128 b = _mm_cvtsi64x_ss(_mm_set1_ps(7.0f), -9000000000LL);
    __m128i v = _mm_set_epi64x(0x1122334455667788LL, -5);
    __m128i l = _mm_setl_epi64(v);
    __int64 slot[2] = { 0, 0 };
    if (_mm_cvtss_si64x(a) != -2) return 1;          /* round to even */
    if (_mm_cvttss_si64x(a) != -2) return 2;         /* truncate */
    if (_mm_cvtss_f32(b) != -9000000000.0f || b.m128_f32[3] != 7.0f) return 3;
    if (l.m128i_i64[0] != -5 || l.m128i_i64[1] != 0) return 4;
    _mm_stream_si64x(&slot[1], 0x0123456789abcdefLL);
    _mm_sfence();
    if (slot[0] != 0 || slot[1] != 0x0123456789abcdefLL) return 5;
    _mm_stream_si64(&slot[0], -1);                   /* the alias */
    _mm_sfence();
    if (slot[0] != -1) return 6;
    return 0;
}