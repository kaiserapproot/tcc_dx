/* Declaring a function type inside a block - typically a function pointer
   `int (*fp)(T)` - leaves the parameters of that type on the local stack
   (post_type keeps them there for the function's own use).  The scope-exit
   destructor walk took such a T placeholder for a local object and ran
   T's destructor on it: a destructor call on an object that was never
   constructed, once per class-typed parameter, even if the pointer was
   never called or even initialized.  With a member owning a buffer, that
   is free() on garbage.

   The buffers are tracked in a table, so a release of a pointer that was
   never acquired is reported instead of corrupting the heap.  The first
   blocks involve no copies, so their destructor counts are exact; the
   blocks that call through the pointer only check that nothing is
   released twice or without having been acquired. */
#include <stdlib.h>

#define MAX_BUFS 32

static int *bufs[MAX_BUFS];
static int released[MAX_BUFS];
static int nb_bufs;
static int double_release;
static int unknown_release;
static int dtors;

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

struct Res {
    int *p;
    Res() { p = acquire(0); }
    Res(const Res &o) { p = acquire(*o.p); }   /* a copy owns its own buffer */
    ~Res() { release(p); p = 0; ++dtors; }
};

struct Pod { int v; };

int take(Res r) { return *r.p; }
int take_two(Res a, Res b) { return *a.p + *b.p; }
int take_pod(Pod p) { return p.v; }

static int clean(int expected_dtors)
{
    return dtors == expected_dtors && double_release == 0 && unknown_release == 0;
}

int main()
{
    {
        int (*fp)(Res) = take;      /* declared, never called */
        Res x;
        *x.p = 1;
    }
    if (!clean(1))
        return 1;
    {
        int (*fp)(Res);             /* not even initialized */
    }
    if (!clean(1))
        return 2;
    {
        int (*fp)(Res, Res) = take_two;
        Res x, y;
    }
    if (!clean(3))
        return 3;
    {
        Res x;
        {
            int (*fp)(Res) = take;  /* in a nested block */
        }
        if (!clean(3))
            return 4;
    }
    if (!clean(4))
        return 5;
    {
        int (*fp)(Res) = take;
        Res x;
        *x.p = 5;
        if (fp(x) != 5)             /* and it still works when called */
            return 6;
    }
    if (double_release || unknown_release)
        return 7;
    {
        int (*fp)(Pod) = take_pod;  /* trivial parameter: nothing to do */
        Pod p;
        p.v = 2;
        if (fp(p) != 2)
            return 8;
    }
    {
        typedef int (*F)(Res);      /* the same through a typedef */
        F fp = take;
        Res x;
        *x.p = 9;
        if (fp(x) != 9)
            return 9;
    }
    if (double_release || unknown_release)
        return 10;
    /* the exits that use the goto / switch destructor bookkeeping (local
       state ids) rather than the plain scope walk: placeholders have no
       local state id, so they must be skipped there as well */
    dtors = 0;
    {
        int i;
        for (i = 0; i < 2; i++) {
            int (*fp)(Res) = take;
            Res x;
            if (i == 0)
                continue;
            break;
        }
    }
    if (!clean(2))
        return 11;
    dtors = 0;
    {
        int (*fp)(Res) = take;
        Res y;
        goto out;
    }
out:
    if (!clean(1))
        return 12;
    dtors = 0;
    switch (1) {
    case 1: {
        int (*fp)(Res) = take;
        Res z;
        break;
    }
    }
    if (!clean(1))
        return 13;
    return 0;
}
