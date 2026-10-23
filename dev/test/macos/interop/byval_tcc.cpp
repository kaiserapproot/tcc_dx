/* The tcc side of the by-value interop test (see byval.h): main, and the
   functions clang calls back. */
#include <stdio.h>
#include "byval.h"

extern "C" int tcc_take(Tracked t)
{
    *t.p += 1000;
    return *t.p;
}

extern "C" int tcc_take_two(int a, Tracked t1, double d, Tracked t2)
{
    if (a != 1 || d != 2.5)
        return -1;
    return *t1.p * 10 + *t2.p;
}

static int check(const char *what, int cond)
{
    printf("%s %s\n", what, cond && byval_live == 0 && byval_bad == 0 ? "ok" : "BAD");
    return cond && byval_live == 0 && byval_bad == 0;
}

int main()
{
    int ok = 1, got, cond;
    /* the checks run after each block, once the arguments are gone */
    {
        Tracked t;
        *t.p = 7;
        got = clang_take(t);
        cond = got == 1007 && *t.p == 7;
    }
    ok = check("tcc->clang one", cond) && ok;
    {
        Tracked a, b;
        *a.p = 4;
        *b.p = 5;
        got = clang_take_two(1, a, 2.5, b);
        cond = got == 45 && *a.p == 4 && *b.p == 5;
    }
    ok = check("tcc->clang two", cond) && ok;
    {
        Pod p;
        p.a = 1; p.b = 2; p.c = 3;
        ok = check("tcc->clang pod", clang_pod(p) == 6) && ok;
    }
    got = clang_calls(tcc_take, 3);
    ok = check("clang->tcc one", got == 10033) && ok;
    got = clang_calls_two(tcc_take_two, 6);
    ok = check("clang->tcc two", got == 67) && ok;
    return ok ? 0 : 1;
}
