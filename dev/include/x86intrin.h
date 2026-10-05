/* x86intrin.h for the tcc_dx include directory.
 *
 * TCC: NOT supported.  The GCC header provides the SSE/AVX intrinsics (_mm_*)
 * and friends as compiler builtins, which TCC does not implement.  (__rdtsc has
 * a TCC body in psdk_inc/intrin-tcc.h, reached through <intrin.h>.)
 * <intrin.h> skips this file for __TINYC__, and a direct include fails here
 * instead of silently providing nothing.
 *
 * GCC / Clang: this directory can end up in front of the compiler's own
 * headers, so pass the include on to the real <x86intrin.h>. */
#ifdef __TINYC__
#error "x86intrin.h (GCC x86 intrinsics: _mm_* ...) is not supported by this TCC build"
#else
#include_next <x86intrin.h>
#endif
