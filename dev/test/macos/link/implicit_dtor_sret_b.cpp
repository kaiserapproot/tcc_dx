#include "implicit_dtor_sret.h"

static int per_block;

/* values right, and the same number of destructions as the first block
   (copy elision is optional, so the number itself is not fixed) */
static void report(const char *what, int values_ok)
{
    if (per_block == 0)
        per_block = dtors;
    printf("%s %s\n", what,
           values_ok && dtors > 0 && dtors == per_block ? "ok" : "BAD");
    dtors = 0;
}

int main()
{
    int ok;
    {
        Small x = make_small(1);
        ok = x.m.a == 1;
    }
    report("small", ok);
    {
        MemD x = make_mem(10);
        ok = x.a == 10 && x.m.a == 11;
    }
    report("mem", ok);
    {
        Base24 x = make_base24(20);
        ok = x.a == 20 && x.b == 21 && x.c == 22;
    }
    report("base24", ok);
    return 0;
}
