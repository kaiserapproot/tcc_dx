/* Second TU, headers in the opposite order. */
#include <intrin.h>
#include <stdlib.h>
#include <windows.h>
int sdk_intrin_tu2(int stop)
{
    if (stop) __debugbreak();
    return _abs64(-7) == 7 ? 0 : 2;
}
