/* An intrinsic without a TCC body is a declaration only.  Using it must fail
   at link time instead of appearing to work.  _InterlockedIncrement and then
   _BitScanForward used to be the example; both have a body now
   (psdk_inc/intrin-tcc.h).  _bittest is not on the list of intrinsics to
   implement next; if it gains a body, move this case to the execution gate
   and pick another intrinsic without one. */
#include <windows.h>
#include <intrin.h>
int main(void)
{
    LONG v = 8;
    return _bittest(&v, 3) ? 0 : 1;
}
