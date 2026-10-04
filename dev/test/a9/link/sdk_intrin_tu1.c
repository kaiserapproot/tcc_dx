/* Two C translation units that both include <intrin.h> must link.  A TCC
   body that is not the first declaration of its function becomes an external
   definition in every TU ("defined twice"): that happened to _abs64. */
#include <windows.h>
#include <stdlib.h>
#include <intrin.h>
int sdk_intrin_tu2(int stop);
int main(void)
{
    if (_abs64(-5) != 5) return 1;
    return sdk_intrin_tu2(0);
}
