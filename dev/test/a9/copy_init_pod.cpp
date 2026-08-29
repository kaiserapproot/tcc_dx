// Non-regression for the shapes FEAT-COPY-INIT must NOT change: a struct
// with no constructor at all keeps the plain struct copy, and a class that
// has a ctor but no copy ctor keeps the same field values (its implicit
// copy has no class-typed member to rebuild).
#include <stdio.h>

struct Pod {
    int a;
    int b;
};

struct WithCtor {
    int v;
    WithCtor() { v = 5; }
};

int main()
{
    Pod p1;
    WithCtor w1;

    p1.a = 3;
    p1.b = 4;
    {
        Pod p2 = p1;

        if (p2.a != 3 || p2.b != 4)
            return 1;
    }
    w1.v = 9;
    {
        WithCtor w2 = w1;

        if (w2.v != 9)
            return 2;
    }
    printf("ok\n");
    return 0;
}