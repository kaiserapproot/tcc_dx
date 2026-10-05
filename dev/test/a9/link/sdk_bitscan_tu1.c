/* Two translation units that both use the bit scan intrinsics and __rdtsc
   must link: the TCC bodies are static __inline__, so neither TU may emit a
   global _BitScanForward & co. ("defined twice").  This TU includes only
   <windows.h>; sdk_bitscan_tu2.c also includes <intrin.h>, which declares
   some of them again after the bodies.
   Built as C here and as C++ by sdk_bitscan_tu1_cpp.cpp. */
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
int bitscan_tu2(unsigned __int64 t0);
#ifdef __cplusplus
}
#endif

int main(void)
{
    unsigned long idx = 0;
    unsigned __int64 t0;
    if (!_BitScanForward(&idx, 0x10UL) || idx != 4) return 1;
    if (!_BitScanReverse64(&idx, 0x10ULL << 40) || idx != 44) return 2;
    t0 = __rdtsc();
    if (bitscan_tu2(t0) != 0) return 3;
    return 0;
}
