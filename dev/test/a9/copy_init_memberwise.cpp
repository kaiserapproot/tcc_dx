// FEAT-COPY-INIT, memberwise half: a class with NO user-declared copy ctor
// still has the C++98 implicit one, and that one is memberwise.  A flat
// byte copy would alias the member's heap buffer into both objects and the
// second destructor would free it twice - the BUG-46 failure, here on the
// stack path instead of `new T(obj)`.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Str {
    char *p;
    Str() { p = (char *)malloc(8); p[0] = 'A'; p[1] = 0; }
    Str(const Str& o) { p = (char *)malloc(8); strcpy(p, o.p); }
    ~Str() { free(p); }
};

struct Holder {
    Str s;
    int n;
    Holder() { n = 7; }
    ~Holder() {}
};

int main()
{
    Holder h;

    {
        Holder g = h;

        g.s.p[0] = 'B';
        if (g.n != 7)
            return 1;
        if (g.s.p == h.s.p)
            return 2;
        printf("g=%s h=%s\n", g.s.p, h.s.p);
    }
    printf("after=%s\n", h.s.p);
    if (h.s.p[0] != 'A')
        return 3;
    return 0;
}