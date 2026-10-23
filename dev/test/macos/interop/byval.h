/* Shared by byval_clang.cpp (built by clang++ into a dylib) and
   byval_tcc.cpp (built by tcc, holds main).  A class with a copy
   constructor and a destructor is passed by value in both directions; the
   Itanium C++ ABI says the caller builds the argument object, passes its
   address and destroys it.  The counters live in the dylib and both sides'
   inline constructors update them, so a callee that wrongly destroyed its
   parameter, or a caller that did not, shows up as live != 0 or as a
   second release of the same buffer. */
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
extern int byval_live;
extern int byval_bad;           /* a buffer released twice */
int *byval_acquire(int v);
void byval_release(int *p);
#ifdef __cplusplus
}
#endif

struct Tracked {
    int *p;
    Tracked() { p = byval_acquire(0); ++byval_live; }
    Tracked(const Tracked &o) { p = byval_acquire(*o.p); ++byval_live; }
    ~Tracked() { byval_release(p); p = 0; --byval_live; }
};

struct Pod { int a, b, c; };

extern "C" {
/* defined in the clang side */
int clang_take(Tracked t);
int clang_take_two(int a, Tracked t1, double d, Tracked t2);
int clang_pod(Pod p);
int clang_calls(int (*fn)(Tracked), int v);          /* clang -> tcc */
int clang_calls_two(int (*fn)(int, Tracked, double, Tracked), int v);
}
