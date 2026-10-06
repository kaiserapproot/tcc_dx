/* Two translation units that both use the SSE / SSE2 intrinsics must link:
   the bodies are static __inline__, so neither TU may emit a global
   _mm_add_ps & co. ("defined twice").  This TU gets them through <intrin.h>,
   simd_tu2.c includes <emmintrin.h> directly; __m128 / __m128i values cross
   between the two by value.  Built as C here and as C++ by simd_tu1_cpp.cpp. */
#include <windows.h>
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif
__m128 simd_tu2_scale(__m128 a, float k);
__m128i simd_tu2_mix(__m128i a, __m128i b);
#ifdef __cplusplus
}
#endif

int main(void)
{
    __m128 a = _mm_setr_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 r = _mm_add_ps(simd_tu2_scale(a, 2.0f), a);
    __m128i x = _mm_setr_epi32(1, 2, 3, 4), y = _mm_set1_epi32(10);
    __m128i z = simd_tu2_mix(x, y);
    if (_mm_movemask_ps(_mm_cmpeq_ps(r, _mm_setr_ps(3.0f, 6.0f, 9.0f, 12.0f))) != 0xf) return 1;
    z = _mm_shuffle_epi32(z, 0x1b);
    if (_mm_cvtsi128_si32(z) != (4 * 2 + 10)) return 2;
    if (_mm_extract_epi16(_mm_slli_epi32(x, 4), 6) != 64) return 3;
    return 0;
}