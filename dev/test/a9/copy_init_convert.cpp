// FEAT-COPY-INIT keeps the non-same-class initializers on the ordinary
// checked-assignment path, where G-CONV's converting constructor applies
// exactly once.  Both forms were a syntax error on master, so this only
// pins down the behaviour that the new `=` branch must not disturb.
#include <stdio.h>

struct S {
    int s;
    S() { s = 4; }
};

struct P {
    int v;
    P() { v = 1; }
    P(int x) { v = x + 7; }
    P(const S& o) { v = o.s + 20; }
};

int main()
{
    S s;
    P a = 5;
    P b = s;

    printf("a=%d b=%d\n", a.v, b.v);
    if (a.v != 12)
        return 1;
    if (b.v != 24)
        return 2;
    return 0;
}