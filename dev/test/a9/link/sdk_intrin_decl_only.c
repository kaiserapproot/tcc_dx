/* An intrinsic without a TCC body is a declaration only.  Using it must fail
   at link time instead of appearing to work.  _InterlockedIncrement used to be
   the example; it has a body now (psdk_inc/intrin-tcc.h).  If TCC gains an
   implementation of _BitScanForward, move this case to the execution gate
   and pick another intrinsic without a body. */
#include <windows.h>
#include <intrin.h>
int main(void)
{
    unsigned long idx = 0;
    return _BitScanForward(&idx, 8) && idx == 3 ? 0 : 1;
}
