/* Classes whose destructor is implicit (from a member or a base).  The
   defining TU (_a.cpp) and the calling TU (_b.cpp) must agree that they
   are returned through the hidden pointer, although _b.cpp only ever sees
   the declarations and never destroys one before the first call.
   int fields keep the sizes the same on SysV (long = 8) and Windows x64
   (long = 4); the typedefs fail to compile if a size drifts. */
#include <stdio.h>

extern int dtors;

struct HasD { int a, pad; ~HasD() { ++dtors; } };  /* 8 bytes */

struct Small  { HasD m; };                /*  8 bytes */
struct MemD   { int a, b; HasD m; };      /* 16 bytes */
struct Base24 : HasD { int b, c, d, e; }; /* 24 bytes */

typedef char check_hasd[sizeof(HasD) == 8 ? 1 : -1];
typedef char check_small[sizeof(Small) == 8 ? 1 : -1];
typedef char check_memd[sizeof(MemD) == 16 ? 1 : -1];
typedef char check_base24[sizeof(Base24) == 24 ? 1 : -1];

Small make_small(long v);
MemD make_mem(long v);
Base24 make_base24(long v);
