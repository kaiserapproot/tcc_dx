/* SSE alone: <xmmintrin.h> without <emmintrin.h>, nothing else included. */
#include <xmmintrin.h>

int main(void)
{
    __m128 a = _mm_setr_ps(4.0f, 9.0f, 16.0f, 25.0f);
    __m128 r = _mm_sqrt_ps(a);
    __m128 s = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 1, 2, 3));
    float out[4];
    _mm_storeu_ps(out, s);
    if (out[0] != 5.0f || out[1] != 4.0f || out[2] != 3.0f || out[3] != 2.0f) return 1;
    if (_mm_cvtss_si32(_mm_set_ss(2.5f)) != 2) return 2;
    return 0;
}