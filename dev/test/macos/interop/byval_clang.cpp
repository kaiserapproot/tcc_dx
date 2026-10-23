/* The clang++ side of the by-value interop test (see byval.h). */
#include "byval.h"

#define MAX_BUFS 64
static int *bufs[MAX_BUFS];
static int released[MAX_BUFS];
static int nb_bufs;

int byval_live;
int byval_bad;

int *byval_acquire(int v)
{
    int *p = (int *)malloc(sizeof(int));
    *p = v;
    if (nb_bufs < MAX_BUFS)
        bufs[nb_bufs++] = p;
    return p;
}

void byval_release(int *p)
{
    int i;
    for (i = 0; i < nb_bufs; i++) {
        if (bufs[i] == p) {
            if (released[i])
                ++byval_bad;
            released[i] = 1;
            return;
        }
    }
    ++byval_bad;
}

int clang_take(Tracked t)
{
    *t.p += 1000;
    return *t.p;
}

int clang_take_two(int a, Tracked t1, double d, Tracked t2)
{
    if (a != 1 || d != 2.5)
        return -1;
    return *t1.p * 10 + *t2.p;
}

int clang_pod(Pod p)
{
    return p.a + p.b + p.c;
}

int clang_calls(int (*fn)(Tracked), int v)
{
    Tracked t;
    int r;
    *t.p = v;
    r = fn(t);
    return r * 10 + *t.p;       /* the callee's change must not reach t */
}

int clang_calls_two(int (*fn)(int, Tracked, double, Tracked), int v)
{
    Tracked a, b;
    *a.p = v;
    *b.p = v + 1;
    return fn(1, a, 2.5, b);
}
