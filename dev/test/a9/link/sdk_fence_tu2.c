/* Second TU of sdk_fence_tu1.c: the same fences again, after <intrin.h>. */
#include <windows.h>
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif
int fence_tu2(void);
#ifdef __cplusplus
}
#endif

int fence_tu2(void)
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
    v = 2;
    return v == 2 ? 0 : 1;
}
