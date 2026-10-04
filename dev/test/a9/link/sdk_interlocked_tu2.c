/* Second TU of sdk_interlocked_tu1.c: the same intrinsics again.
   In C++ the function is declared extern "C" and defined outside the block,
   the usual header / source split: the definition keeps the C linkage, and
   the calls in it are overload-resolved (winbase.h has C++ overloads of the
   Interlocked names).  Both used to go wrong in TCC. */
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
extern volatile LONG g_shared;
int interlocked_tu2(void);
#ifdef __cplusplus
}
#endif

int interlocked_tu2(void)
{
    LONG64 w = 5;
    PVOID p = NULL;
    if (InterlockedIncrement(&g_shared) != 2) return 1;
    if (InterlockedExchangeAdd(&g_shared, 1) != 2) return 2;
    if (InterlockedIncrement64(&w) != 6) return 3;
    if (InterlockedCompareExchangePointer(&p, &w, NULL) != NULL || p != (PVOID)&w) return 4;
    return 0;
}
