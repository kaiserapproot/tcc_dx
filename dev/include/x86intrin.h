/* Stub x86intrin.h for TCC.
 * tcc_dx include/_mingw.h defines __GNUC__ 4 for mingw compatibility, which makes
 * include/intrin.h pull <x86intrin.h>. Real GCC header is not shipped with TCC.
 * Empty guard is enough for Amateras Fiber coroutine / windows.h consumers. */
#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED
#endif