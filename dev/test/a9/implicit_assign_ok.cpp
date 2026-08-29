// The implicit-copy-assignment guard must leave every trivial shape alone.
// A struct assignment stays a plain flat copy unless some member declares
// its own operator=, and a class that declares operator= (its own or on a
// member, with the outer class defining one too) keeps running it.
#include <stdio.h>

struct Pod {
    int a;
    int b;
};

struct Plain {
    int a;
    int b;
    Plain() { a = 0; b = 0; }
};

struct M {
    int v;
    M() { v = 1; }
    M& operator=(const M& o) { v = o.v + 1000; return *this; }
};

struct Outer {
    M m;
    int n;
    Outer() { n = 0; }
    Outer& operator=(const Outer& o) { m = o.m; n = o.n + 5; return *this; }
};

int main()
{
    Pod p1, p2;
    Plain q1, q2;
    M a, b;
    Outer h1, h2;

    p1.a = 1;
    p1.b = 2;
    p2 = p1;
    q1.a = 4;
    q1.b = 6;
    q2 = q1;
    a.v = 5;
    b = a;
    h1.m.v = 7;
    h1.n = 3;
    h2 = h1;
    printf("pod=%d,%d plain=%d,%d m=%d outer=%d,%d\n",
           p2.a, p2.b, q2.a, q2.b, b.v, h2.m.v, h2.n);
    if (p2.a != 1 || p2.b != 2)
        return 1;
    if (q2.a != 4 || q2.b != 6)
        return 2;
    if (b.v != 1005)
        return 3;
    if (h2.m.v != 1007 || h2.n != 8)
        return 4;
    return 0;
}