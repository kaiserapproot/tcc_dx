/* C shared library that registers an atexit() handler from inside the
   .so.  After dlclose() the handler must not be left pointing into the
   unmapped library (glibc runs it at dlclose through __cxa_finalize). */
#include <stdio.h>
#include <stdlib.h>
static void so_cleanup(void) { fputs("SO_ATEXIT\n", stderr); }
void so_initialize(void) { atexit(so_cleanup); }
