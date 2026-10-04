// System-header side of the BUG-52 gate boundary.
//
// cpp_allow_local_class_direct_init() now returns 1 for every non-synthetic
// file, which includes the SDK headers.  A C system header has no class with a
// constructor, so the widened gate should have nothing to act on there; this
// file is the check that pulling in a large set of them still parses and links
// in C++ mode, and that a local class initialization written after them still
// takes the C++ path.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <windows.h>

struct S {
    int v;
    S() { v = 5; }
    S(const S &o) { v = o.v + 10; }
    ~S() { }
};

static S make_s(void)
{
    S s;
    return s;
}

int main(void)
{
    S a;
    S b(a);              /* FEAT-4B after the system headers */
    S c = make_s();      /* FEAT-COPY-INIT after the system headers */
    DWORD tick = GetTickCount();

    printf("b=%d c=%d tick_nonzero=%d\n", b.v, c.v, tick != 0);

    if (b.v != 15) return 1;   /* copy ctor ran once */
    if (c.v != 25) return 2;   /* return copy + copy-init */
    return 0;
}
