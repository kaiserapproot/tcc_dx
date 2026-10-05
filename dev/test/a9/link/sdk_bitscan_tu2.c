/* Second TU of sdk_bitscan_tu1.c: the same intrinsics again, after
   <intrin.h> as well. */
#include <windows.h>
#include <intrin.h>

#ifdef __cplusplus
extern "C" {
#endif
int bitscan_tu2(unsigned __int64 t0);
#ifdef __cplusplus
}
#endif

int bitscan_tu2(unsigned __int64 t0)
{
    unsigned long idx = 0;
    if (!_BitScanReverse(&idx, 0x10UL) || idx != 4) return 1;
    if (!_BitScanForward64(&idx, 0x10ULL << 40) || idx != 44) return 2;
    if (__rdtsc() < t0) return 3;
    return 0;
}
