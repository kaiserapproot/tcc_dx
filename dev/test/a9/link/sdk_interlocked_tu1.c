/* Two translation units that both include <windows.h> and both call the
   Interlocked intrinsics must link: the TCC bodies are static __inline__, so
   neither TU may emit a global _InterlockedIncrement & co. ("defined twice").
   Both TUs also work on one shared counter, so the calls really happen.
   Built as C here and as C++ by sdk_interlocked_tu1_cpp.cpp. */
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
extern volatile LONG g_shared;
int interlocked_tu2(void);
#ifdef __cplusplus
}
#endif

volatile LONG g_shared;

int main(void)
{
    LONG64 w = 1;
    PVOID p = NULL;
    if (InterlockedIncrement(&g_shared) != 1) return 1;
    if (InterlockedIncrement64(&w) != 2) return 2;
    if (InterlockedCompareExchangePointer(&p, &w, NULL) != NULL || p != (PVOID)&w) return 3;
    if (interlocked_tu2() != 0) return 4;
    if (g_shared != 3) return 5;
    return 0;
}
