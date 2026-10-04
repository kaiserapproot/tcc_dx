/* <intrin.h> on its own must compile, link and run from C.  It used to stop
   at _mingw.h's __debugbreak: an extern prototype, a static __inline__ body
   and intrin.h's re-declaration together forced the GCC-asm body to be
   compiled. */
#include <intrin.h>
int main(void) { return 0; }
