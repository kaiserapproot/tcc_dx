/* Classes whose destructor is implicit (from a member or a base).  The
   defining TU (_a.cpp) and the calling TU (_b.cpp) must agree that they
   are returned through the hidden pointer, although _b.cpp only ever sees
   the declarations and never destroys one before the first call. */
#include <stdio.h>

extern int dtors;

struct HasD { long a; ~HasD() { ++dtors; } };

struct Small  { HasD m; };            /*  8 bytes */
struct MemD   { long a; HasD m; };    /* 16 bytes */
struct Base24 : HasD { long b, c; };  /* 24 bytes */

Small make_small(long v);
MemD make_mem(long v);
Base24 make_base24(long v);
