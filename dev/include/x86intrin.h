/* x86intrin.h is NOT supported by this TCC build.
 * The GCC header provides the SSE/AVX intrinsics (_mm_*), __rdtsc and friends as
 * compiler builtins, none of which TCC implements.  This file used to be an
 * empty stub so that <intrin.h> could be included; <intrin.h> now skips it for
 * __TINYC__, and a direct include fails here instead of silently providing
 * nothing. */
#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED
#error "x86intrin.h (GCC x86 intrinsics: _mm_*, __rdtsc, ...) is not supported by this TCC build"
#endif
