/* C++ global with a destructor in a shared library: must be rejected
   (no __cxa_finalize at dlclose, see tccelf.c tcc_add_cpp_elf_runtime) */
#include <stdio.h>
struct G { int v; G() { v = 7; } ~G() { fputs("LIB_GLOBAL_DTOR\n", stderr); } };
G g;
extern "C" int lib_get() { return g.v; }
