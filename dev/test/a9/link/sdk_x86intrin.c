/* A direct include must be rejected: TCC provides none of the GCC x86
   intrinsics, so an empty header would only pretend to support them. */
#include <x86intrin.h>
int main(void) { return 0; }
