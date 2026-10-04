/* x86intrin.h for the tcc_dx include directory.
 *
 * TCC: NOT supported.  The GCC header provides the SSE/AVX intrinsics (_mm_*),
 * __rdtsc and friends as compiler builtins, none of which TCC implements.
 * <intrin.h> skips this file for __TINYC__, and a direct include fails here
 * instead of silently providing nothing.
 *
 * GCC / Clang: this directory can end up in front of the compiler's own
 * headers, so pass the include on to the real <x86intrin.h>. */
#ifdef __TINYC__
#error "x86intrin.h (GCC x86 intrinsics: _mm_*, __rdtsc, ...) is not supported by this TCC build"
#else
#include_next <x86intrin.h>
#endif
