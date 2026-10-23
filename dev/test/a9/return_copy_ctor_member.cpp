/* Returning a class by value copied the local into the return slot as a
   flat byte copy, without running the copy constructors of its members
   and bases (the hidden-pointer path lacked the memberwise repair that
   copy-initialization has, and a class with no destructor of its own was
   not routed through the repair at all).  A member owning a buffer was
   then aliased: two objects freed the same pointer.

   Copy elision is optional in C++98, so the test never fixes the number
   of copies.  It observes the copy constructors in three ways that do not
   depend on elision:
   - every buffer is tracked in a table instead of being freed, so a
     second destruction of the same buffer is reported instead of
     corrupting the heap, and a copy must own a different buffer;
   - every constructor (default and copy) records the address it ran on,
     so the object that finally exists must have been constructed there
     by one of them - a flat byte copy constructs nothing;
   - the copy constructors of a base and two members log their order,
     which must be base first, then members in declaration order, for
     every copy that was not elided. */
#include <stdlib.h>
#include <string.h>

#define MAX_BUFS 64

static int *bufs[MAX_BUFS];
static int released[MAX_BUFS];
static int nb_bufs;
static int double_release;
static int unknown_release;
static int live;

/* addresses the constructors ran on, cleared before each block */
static const void *built[MAX_BUFS];
static int nb_built;

static void constructed(const void *p)
{
    if (nb_built < MAX_BUFS)
        built[nb_built++] = p;
}

static void forget_constructed(void)
{
    nb_built = 0;
}

static int constructed_at(const void *p)
{
    int i;
    for (i = 0; i < nb_built; i++)
        if (built[i] == p)
            return 1;
    return 0;
}

static int *acquire(int v)
{
    int *p = (int *)malloc(sizeof(int));
    *p = v;
    if (nb_bufs < MAX_BUFS)
        bufs[nb_bufs++] = p;
    return p;
}

static void release(int *p)
{
    int i;
    for (i = 0; i < nb_bufs; i++) {
        if (bufs[i] == p) {
            if (released[i])
                ++double_release;
            released[i] = 1;
            return;
        }
    }
    ++unknown_release;
}

/* a member that owns a buffer: copying must allocate a new one */
struct Res {
    int *p;
    Res() { p = acquire(0); ++live; constructed(this); }
    Res(const Res &o) { p = acquire(*o.p); ++live; constructed(this); }
    ~Res() { release(p); p = 0; --live; }
};

struct Wrap   { int tag; Res m; };            /* via a member, 16 bytes */
struct Deep   { Wrap w; int pad; };           /* nested two levels, 24 bytes */
struct ByBase : Res { int tag; };             /* via a base */
struct OwnD   { int tag; Res m; ~OwnD() {} }; /* own destructor, implicit copy */

/* no destructor anywhere, so no double release is possible; the copy
   constructor must still be honoured on the way out of the callee, which
   only the constructed-address record can show */
struct OnlyCopy {
    int a;
    OnlyCopy() { a = 0; constructed(this); }
    OnlyCopy(const OnlyCopy &o) { a = o.a; constructed(this); }
};
struct W8 { OnlyCopy m; };                    /* 4 bytes, returned in a register */

/* order of the copy constructors: base, then members as declared */
static char order_log[32];
static void order(char c)
{
    size_t n = strlen(order_log);
    if (n + 1 < sizeof order_log) {
        order_log[n] = c;
        order_log[n + 1] = 0;
    }
}
struct OB { int b; OB() { b = 0; } OB(const OB &o) { b = o.b; order('B'); } };
struct O1 { int v; O1() { v = 0; } O1(const O1 &o) { v = o.v; order('1'); } };
struct O2 { int v; O2() { v = 0; } O2(const O2 &o) { v = o.v; order('2'); } };
struct Ordered : OB { O1 m1; O2 m2; };

/* "" (elided), "B12", "B12B12", ...: whole copies, each in order */
static int order_log_ok(void)
{
    size_t n = strlen(order_log), i;
    if (n % 3 != 0)
        return 0;
    for (i = 0; i < n; i += 3)
        if (order_log[i] != 'B' || order_log[i + 1] != '1' || order_log[i + 2] != '2')
            return 0;
    return 1;
}

Wrap make_wrap(int v) { Wrap r; r.tag = v; *r.m.p = v + 1; return r; }
Deep make_deep(int v) { Deep r; r.w.tag = v; *r.w.m.p = v + 1; r.pad = v + 2; return r; }
ByBase make_bybase(int v) { ByBase r; r.tag = v; *r.p = v + 1; return r; }
OwnD make_ownd(int v) { OwnD r; r.tag = v; *r.m.p = v + 1; return r; }
Wrap forward_wrap(int v) { return make_wrap(v); }   /* returns a call result */
W8 make_w8(int v) { W8 r; r.m.a = v; return r; }
W8 forward_w8(int v) { return make_w8(v); }
Ordered make_ordered(int v) { Ordered r; r.b = v; r.m1.v = v + 1; r.m2.v = v + 2; return r; }

static int all_distinct(void)
{
    int i, j;
    for (i = 0; i < nb_bufs; i++)
        for (j = i + 1; j < nb_bufs; j++)
            if (bufs[i] == bufs[j])
                return 0;
    return 1;
}

/* after a block: everything destroyed once, nothing released twice */
static int balanced(void)
{
    int i;
    if (live != 0 || double_release || unknown_release)
        return 0;
    for (i = 0; i < nb_bufs; i++)
        if (!released[i])
            return 0;
    return 1;
}

int main()
{
    int ok = 1;
    forget_constructed();
    {
        Wrap x = make_wrap(10);
        ok = ok && x.tag == 10 && *x.m.p == 11 && constructed_at(&x.m);
    }
    if (!ok || !balanced())
        return 1;
    forget_constructed();
    {
        Deep x = make_deep(20);
        ok = ok && x.w.tag == 20 && *x.w.m.p == 21 && x.pad == 22
             && constructed_at(&x.w.m);
    }
    if (!ok || !balanced())
        return 2;
    forget_constructed();
    {
        ByBase x = make_bybase(30);
        ok = ok && x.tag == 30 && *x.p == 31 && constructed_at((Res *)&x);
    }
    if (!ok || !balanced())
        return 3;
    forget_constructed();
    {
        OwnD x = make_ownd(40);
        ok = ok && x.tag == 40 && *x.m.p == 41 && constructed_at(&x.m);
    }
    if (!ok || !balanced())
        return 4;
    forget_constructed();
    {
        Wrap x = forward_wrap(50);
        ok = ok && x.tag == 50 && *x.m.p == 51 && constructed_at(&x.m);
    }
    if (!ok || !balanced())
        return 5;
    make_wrap(60);                      /* discarded result */
    if (!balanced())
        return 6;
    forget_constructed();
    {
        /* two live objects from the same source value: separate buffers */
        Wrap a = make_wrap(70);
        Wrap b = a;
        *b.m.p = 99;
        ok = ok && *a.m.p == 71 && *b.m.p == 99 && a.m.p != b.m.p
             && constructed_at(&a.m) && constructed_at(&b.m);
    }
    if (!ok || !balanced())
        return 7;
    if (!all_distinct())
        return 8;
    forget_constructed();
    {
        /* register-returned class without any destructor */
        W8 x = make_w8(80);
        W8 y = forward_w8(81);
        ok = ok && x.m.a == 80 && y.m.a == 81
             && constructed_at(&x.m) && constructed_at(&y.m);
    }
    if (!ok)
        return 9;
    {
        /* base first, then members in declaration order, per copy */
        Ordered x = make_ordered(90);
        ok = ok && x.b == 90 && x.m1.v == 91 && x.m2.v == 92 && order_log_ok();
        order_log[0] = 0;
        {
            /* an lvalue copy cannot be elided: exactly one ordered copy */
            Ordered y = x;
            ok = ok && y.b == 90 && y.m2.v == 92 && strcmp(order_log, "B12") == 0;
        }
    }
    if (!ok)
        return 10;
    return 0;
}
