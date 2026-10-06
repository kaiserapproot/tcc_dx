/* Second TU of simd_tu1.c: the intrinsics again, from <emmintrin.h> alone. */
#include <emmintrin.h>

#ifdef __cplusplus
extern "C" {
#endif
__m128 simd_tu2_scale(__m128 a, float k);
__m128i simd_tu2_mix(__m128i a, __m128i b);
#ifdef __cplusplus
}
#endif

__m128 simd_tu2_scale(__m128 a, float k)
{
    return _mm_mul_ps(a, _mm_set1_ps(k));
}

__m128i simd_tu2_mix(__m128i a, __m128i b)
{
    return _mm_add_epi32(_mm_slli_epi32(a, 1), b);
}