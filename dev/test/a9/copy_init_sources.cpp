// FEAT-COPY-INIT: the initializer of a copy-init can be any lvalue/rvalue of
// the same class - a reference, a dereferenced pointer, or a function return.
// All of them went through the plain struct assignment before the fix
// (measured on master: 3 / 3 / 5 instead of 103 / 103 / 105).
// b3 is built from a function return: C++98 lets the compiler elide the
// copy out of make() and the copy into b3, so 0, 1 or 2 copies are all
// legal (5 / 105 / 205).  Only the lvalue copies b1 / b2 are fixed at one.
#include <stdio.h>

struct P {
    int v;
    P() { v = 1; }
    P(const P& o) { v = o.v + 100; }
};

static P make(void)
{
    P t;

    t.v = 5;
    return t;
}

int main()
{
    P a;
    P *p;

    a.v = 3;
    p = &a;
    {
        P &r = a;
        P b1 = r;
        P b2 = *p;
        P b3 = make();

        printf("b1=%d b2=%d b3=%d\n", b1.v, b2.v, b3.v);
        if (b1.v != 103)
            return 1;
        if (b2.v != 103)
            return 2;
        if (b3.v != 5 && b3.v != 105 && b3.v != 205)
            return 3;
    }
    return 0;
}