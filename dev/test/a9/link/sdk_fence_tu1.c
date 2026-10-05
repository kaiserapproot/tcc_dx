/* Two translation units that both use the fences must link: the TCC bodies
   are static __inline__, so neither TU may emit a global _mm_mfence & co.
   ("defined twice").  sdk_fence_tu2.c also includes <intrin.h>, which on
   x86-64 declares _mm_lfence, _mm_mfence and _mm_pause again after the
   bodies (a plain prototype after a static definition).
   Built as C here and as C++ by sdk_fence_tu1_cpp.cpp. */
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
int fence_tu2(void);
#ifdef __cplusplus
}
#endif

int main(void)
{
    volatile LONG v = 0;
    _mm_mfence();
    _mm_lfence();
    _mm_sfence();
    _mm_pause();
    __faststorefence();
    MemoryBarrier();
    YieldProcessor();
    _ReadWriteBarrier();
    v = 1;
    if (v != 1) return 1;
    if (fence_tu2() != 0) return 2;
    return 0;
}
