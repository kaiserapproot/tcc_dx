/* A class with a non-trivial copy constructor or destructor passed by
   value used to be copied into the argument area as plain bytes: no copy
   constructor ran, the parameter was never destroyed, and a member owning
   a buffer was aliased between the argument and the parameter.

   Now (Itanium C++ ABI 3.1.2.3) the caller copy-constructs the argument
   object into a temporary, passes its address, and destroys it at the end
   of the full expression; the callee reads the parameter through that
   address.  The test observes this without fixing the number of copies:
   every buffer is tracked so a double release or a leak is reported, every
   constructor records the address it ran on, and the callee reports the
   address and value it saw. */
#include <stdlib.h>

#define MAX_BUFS 64

static int *bufs[MAX_BUFS];
static int released[MAX_BUFS];
static int nb_bufs;
static int double_release;
static int unknown_release;
static int live;

static const void *built[MAX_BUFS];
static int nb_built;

static void constructed(const void *p)
{
    if (nb_built < MAX_BUFS)
        built[nb_built++] = p;
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

/* after a statement: everything destroyed once, nothing released twice */
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

struct Res {
    int *p;
    Res() { p = acquire(0); ++live; constructed(this); }
    Res(const Res &o) { p = acquire(*o.p); ++live; constructed(this); }
    ~Res() { release(p); p = 0; --live; }
};

/* implicit copy constructor and destructor, through the member */
struct Wrap { int tag; Res m; };

/* no destructor: only the constructed-address record can see the copy */
struct OnlyCopy {
    int a;
    OnlyCopy() { a = 0; constructed(this); }
    OnlyCopy(const OnlyCopy &o) { a = o.a; constructed(this); }
};

/* trivial: must keep the ordinary struct passing */
struct Pod { int a, b, c; };

struct Shape { virtual int kind() { return 1; } };
struct Circle : Shape { int kind() { return 2; } };

static const void *seen_param;

int take(Res r)
{
    seen_param = &r;
    *r.p += 1000;                       /* must not reach the caller */
    return *r.p;
}

int take_two(int a, Res r1, long b, Res r2, int c)
{
    if (a != 1 || b != 2 || c != 3)
        return -1;
    return *r1.p * 10 + *r2.p;
}

int early(Res r, int flag)
{
    if (flag)
        return *r.p;                    /* early return: the caller still destroys */
    *r.p = 0;
    return -*r.p;
}

Res make(int v) { Res r; *r.p = v; return r; }

int nested(int v) { return take(make(v)); }

int take_wrap(Wrap w) { return w.tag * 100 + *w.m.p; }

int take_only_copy(OnlyCopy o) { return constructed_at(&o) ? o.a : -1; }

int take_pod(Pod p) { return p.a + p.b + p.c; }

int kind_of(Shape s) { return s.kind(); }

struct Holder {
    int v;
    int add(Res r) { return v + *r.p; }
};

/* at file scope: a function-pointer declaration inside a block whose
   parameter list names a class makes the scope exit destroy the parameter
   placeholder as if it were a local (a separate, pre-existing problem) */
static int (*take_ptr)(Res) = take;

int main()
{
    int ok = 1;
    int got;
    {
        Res x;
        *x.p = 7;
        got = take(x);
        /* the callee saw its own object, built by a constructor, and its
           change stayed there */
        ok = ok && got == 1007 && *x.p == 7
             && seen_param != (const void *)&x && constructed_at(seen_param);
    }
    if (!ok || !balanced())
        return 1;
    {
        Res x, y;
        *x.p = 4; *y.p = 5;
        got = take_two(1, x, 2, y, 3);  /* class args between scalars */
        ok = ok && got == 45 && *x.p == 4 && *y.p == 5;
    }
    if (!ok || !balanced())
        return 2;
    {
        Res x;
        *x.p = 9;
        ok = ok && early(x, 1) == 9 && early(x, 0) == 0 && *x.p == 9;
    }
    if (!ok || !balanced())
        return 3;
    ok = ok && nested(6) == 1006;      /* argument is itself a temporary */
    if (!ok || !balanced())
        return 4;
    ok = ok && take(make(8)) == 1008;
    if (!ok || !balanced())
        return 5;
    {
        Wrap w;
        w.tag = 2;
        *w.m.p = 3;
        ok = ok && take_wrap(w) == 203 && *w.m.p == 3;
    }
    if (!ok || !balanced())
        return 6;
    {
        OnlyCopy o;
        o.a = 11;
        ok = ok && take_only_copy(o) == 11;
    }
    if (!ok)
        return 7;
    {
        Pod p;
        p.a = 1; p.b = 2; p.c = 3;
        ok = ok && take_pod(p) == 6;
    }
    if (!ok)
        return 8;
    {
        Circle c;
        Shape s;
        /* a polymorphic class is passed by invisible reference too (tcc
           rejects slicing a Circle into the Shape parameter, so only a
           Shape is passed) */
        ok = ok && kind_of(s) == 1 && c.kind() == 2;
    }
    if (!ok)
        return 9;
    {
        Holder h;
        Res x;
        h.v = 30;
        *x.p = 12;
        ok = ok && h.add(x) == 42;      /* member function */
    }
    if (!ok || !balanced())
        return 10;
    {
        Res x;
        *x.p = 1;
        ok = ok && take_ptr(x) == 1001 && *x.p == 1;   /* through a pointer */
    }
    if (!ok || !balanced())
        return 11;
    {
        /* a copy-constructed argument and a temporary from a call share
           the full expression: both must be destroyed, once, after the
           call, whichever order they come in */
        Res x;
        *x.p = 2;
        got = take_two(1, x, 2, make(20), 3);
        ok = ok && got == 40 && *x.p == 2;
        got = take_two(1, make(30), 2, x, 3);
        ok = ok && got == 302 && *x.p == 2;
        got = take_two(1, make(4), 2, make(5), 3);
        ok = ok && got == 45;
        ok = ok && live == 1;           /* only x is alive here */
    }
    if (!ok || !balanced())
        return 12;
    return 0;
}
