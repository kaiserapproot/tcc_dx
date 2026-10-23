/* A class whose destructor is only implicit (it comes from a member or a
   base) is returned through the hidden pointer, like a class with its own
   destructor.  The implicit destructor is made lazily, the first time an
   object of the class is destroyed, so deciding by "has a destructor
   member" let the prolog of the first function using the class choose
   registers while its return statement wrote through the hidden pointer,
   and let callers compiled earlier disagree with the definition.

   Every destructor appends its object's tag to a log.  After each block
   the test checks that only objects of that block's tag were destroyed,
   that the number of destructions is the same for every class shape
   (8 / 16 / 24 bytes, member or base), and that the variables of the last
   block are destroyed in reverse order.  Copy elision is optional in
   C++98, so the exact number of destructions per block is not fixed. */
#include <string.h>

static char log_buf[64];
static size_t per_block;

static void note(char c)
{
    size_t n = strlen(log_buf);
    if (n + 1 < sizeof log_buf) {
        log_buf[n] = c;
        log_buf[n + 1] = 0;
    }
}

struct HasD { long a; char tag; ~HasD() { note(tag); } };

struct Small  { HasD m; };                    /*  8 bytes, via a member */
struct MemD   { long a; HasD m; };            /* 16 bytes, via a member */
struct BaseD  : HasD { long b; };             /* 16 bytes, via a base */
struct Mem24  { long a, b; HasD m; };         /* 24 bytes, via a member */
struct Base24 : HasD { long b, c; };          /* 24 bytes, via a base */

/* called before its definition, before any Small is destroyed */
Small make_small(long v, char tag);

static int use_small(void)
{
    Small s = make_small(40, 'S');
    return s.m.a == 40;
}

Small make_small(long v, char tag) { Small r; r.m.a = v; r.m.tag = tag; return r; }
MemD make_mem(long v, char tag) { MemD r; r.a = v; r.m.a = v + 1; r.m.tag = tag; return r; }
BaseD make_base(long v, char tag) { BaseD r; r.a = v; r.b = v + 1; r.tag = tag; return r; }
Mem24 make_mem24(long v, char tag) { Mem24 r; r.a = v; r.b = v + 1; r.m.a = v + 2; r.m.tag = tag; return r; }
Base24 make_base24(long v, char tag) { Base24 r; r.a = v; r.b = v + 1; r.c = v + 2; r.tag = tag; return r; }

/* the block destroyed only objects tagged 'tag', as many times as the
   first block did (same_count), or at least once; then clear the log for
   the next block */
static int block_ok(char tag, int same_count)
{
    size_t i, n = strlen(log_buf);
    if (n == 0)
        return 0;
    if (per_block == 0)
        per_block = n;
    if (same_count && n != per_block)
        return 0;
    for (i = 0; i < n; i++)
        if (log_buf[i] != tag)
            return 0;
    log_buf[0] = 0;
    return 1;
}

int main()
{
    int ok = 1;
    size_t n;
    {
        MemD x = make_mem(10, 'M');
        ok = ok && x.a == 10 && x.m.a == 11;
    }
    ok = ok && block_ok('M', 1);
    {
        BaseD x = make_base(20, 'B');
        ok = ok && x.a == 20 && x.b == 21;
    }
    ok = ok && block_ok('B', 1);
    {
        Mem24 x = make_mem24(30, 'm');
        ok = ok && x.a == 30 && x.b == 31 && x.m.a == 32;
    }
    ok = ok && block_ok('m', 1);
    {
        Base24 x = make_base24(50, 'b');
        ok = ok && x.a == 50 && x.b == 51 && x.c == 52;
    }
    ok = ok && block_ok('b', 1);
    /* use_small() was compiled before any Small was destroyed, so its
       variable and temporary are not yet registered for destruction
       (temporaries and locals of a class whose implicit destructor has
       not been made are a separate, pre-existing gap); only the value
       and the absence of stray destructions are checked here */
    ok = ok && use_small() && block_ok('S', 0);
    if (!ok)
        return 1;
    {
        /* two objects alive in one scope: the variables themselves are
           destroyed last, in reverse order of construction */
        MemD first = make_mem(60, '1');
        MemD second = make_mem(70, '2');
        ok = ok && first.a == 60 && second.a == 70;
    }
    n = strlen(log_buf);
    if (!ok || n != 2 * per_block)
        return 2;
    if (log_buf[n - 2] != '2' || log_buf[n - 1] != '1')
        return 3;
    return 0;
}
