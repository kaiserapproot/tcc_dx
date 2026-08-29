// FEAT-COPY-INIT: copy-initialization `T b = a;` must run the copy ctor.
// Before the fix this one declaration form fell through to a plain struct
// assignment, so b kept the source's field value unchanged (measured: 1
// instead of 101) while the direct-init form `T c(a);` already worked.
// All three forms are asserted here so a later change cannot repair one
// of them and silently break another.
#include <stdio.h>

static int g_copies = 0;

struct P {
    int v;
    P() { v = 1; }
    P(const P& o) { v = o.v + 100; g_copies++; }
    P& operator=(const P& o) { v = o.v + 1000; return *this; }
};

int main()
{
    P a;
    P b = a;
    P c(a);
    P d;

    d = a;
    printf("b=%d c=%d d=%d copies=%d\n", b.v, c.v, d.v, g_copies);
    if (b.v != 101)
        return 1;
    if (c.v != 101)
        return 2;
    if (d.v != 1001)
        return 3;
    if (g_copies != 2)
        return 4;
    return 0;
}