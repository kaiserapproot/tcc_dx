/* Second TU of sdk_bitscan_tu1.c: the same intrinsics again, after
   <intrin.h> as well. */
#include <windows.h>
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif
int bitscan_tu2(void);
#ifdef __cplusplus
}
#endif

int bitscan_tu2(void)
{
    unsigned long idx = 0;
    volatile unsigned __int64 t;
    if (!_BitScanReverse(&idx, 0x10UL) || idx != 4) return 1;
    if (!_BitScanForward64(&idx, 0x10ULL << 40) || idx != 44) return 2;
    t = __rdtsc();
    (void)t;
    return 0;
}
