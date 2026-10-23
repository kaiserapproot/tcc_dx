/* The implicit destructor of a class (one it gets from a member or a base)
   is made the first time an object of the class is destroyed.  A class
   temporary - the result of a call, mostly - was registered for
   destruction only when the destructor already existed, so the first
   temporary of a class in a TU, before any of its objects had been
   destroyed, was never destroyed: a function compiled before the callee's
   definition leaked every call result.

   Every buffer is tracked in a table; after each full expression every
   buffer acquired so far must have been released (no leak), none twice.
   Copy elision is optional in C++98, so the number of copies, and so of
   buffers, is not fixed; the balance is. */
#include <stdlib.h>

#define MAX_BUFS 64

static int *bufs[MAX_BUFS];
static int released[MAX_BUFS];
static int nb_bufs;
static int double_release;
static int unknown_release;

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

/* everything acquired so far has been released exactly once */
static int settled(void)
{
    int i;
    if (double_release || unknown_release)
        return 0;
    for (i = 0; i < nb_bufs; i++)
        if (!released[i])
            return 0;
    return nb_bufs > 0;
}

struct Res {
    int *p;
    Res() { p = acquire(0); }
    Res(const Res &o) { p = acquire(*o.p); }
    ~Res() { release(p); p = 0; }
};

/* implicit destructors only; each class is first met by the caller below,
   through a declaration, before any object of it has been destroyed */
struct ViaMember { int tag; Res m; };
struct ViaBase : Res { int tag; };
struct Nested { ViaMember inner; int pad; };

ViaMember make_member(int v);
ViaBase make_base(int v);
Nested make_nested(int v);
ViaMember forward(int v);

/* compiled before the definitions: the temporaries here are the first
   objects of their classes in this TU */
int discard(void)
{
    make_member(1);                     /* result thrown away */
    return settled() ? 0 : 1;
}

int in_expression(void)
{
    int v = *make_member(2).m.p;        /* temporary inside an expression */
    if (v != 3)
        return 2;
    return settled() ? 0 : 3;
}

int copy_init(void)
{
    {
        ViaMember x = make_member(4);
        if (x.tag != 4 || *x.m.p != 5)
            return 4;
    }
    return settled() ? 0 : 5;
}

int base_and_nested(void)
{
    {
        ViaBase b = make_base(6);
        Nested n = make_nested(8);
        if (b.tag != 6 || *b.p != 7 || n.inner.tag != 8 || *n.inner.m.p != 9)
            return 6;
    }
    return settled() ? 0 : 7;
}

int through_forward(void)
{
    {
        ViaMember x = forward(10);      /* forward() returns a call result */
        if (x.tag != 10 || *x.m.p != 11)
            return 8;
    }
    return settled() ? 0 : 9;
}

ViaMember make_member(int v) { ViaMember r; r.tag = v; *r.m.p = v + 1; return r; }
ViaBase make_base(int v) { ViaBase r; r.tag = v; *r.p = v + 1; return r; }
Nested make_nested(int v) { Nested r; r.inner.tag = v; *r.inner.m.p = v + 1; r.pad = 0; return r; }
ViaMember forward(int v) { return make_member(v); }

int main()
{
    int rc;
    if ((rc = discard()) != 0)
        return rc;
    if ((rc = in_expression()) != 0)
        return rc;
    if ((rc = copy_init()) != 0)
        return rc;
    if ((rc = base_and_nested()) != 0)
        return rc;
    if ((rc = through_forward()) != 0)
        return rc;
    return 0;
}
