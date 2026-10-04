/* An intrinsic without a TCC body is a declaration only.  Using it must fail
   at link time instead of appearing to work.  If TCC gains an implementation
   of _InterlockedIncrement, move this case to the execution gate. */
#include <windows.h>
#include <intrin.h>
int main(void)
{
    long v = 1;
    return _InterlockedIncrement(&v) == 2 ? 0 : 1;
}
