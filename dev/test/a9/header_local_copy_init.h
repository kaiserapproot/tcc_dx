// Bodies of the local class-initialization test, deliberately placed in an
// INCLUDED file.  cpp_allow_local_class_direct_init() used to require
// file->prev == NULL (primary TU only), so FEAT-4B / FEAT-COPY-INIT were
// skipped here: `T b = make_t();` fell through to the plain C struct
// assignment and `T c(a);` did not parse as a ctor call at all.
//
// Measured on the pre-fix binary (a3eba99):
//   header copy-init  -> value 101, 1 copy ctor call
//   primary copy-init -> value 201, 2 copy ctor calls
//   header direct-init -> "lvalue expected" at compile time
//   header ownership   -> copy aliases the source pointer (g_badfree > 0)
// The absolute counts are a tpp quirk (struct returns move by memcpy); what
// must hold is that an included file behaves exactly like the primary TU, and
// that a class owning heap memory is never copied by aliasing its pointer.
#ifndef HEADER_LOCAL_COPY_INIT_H
#define HEADER_LOCAL_COPY_INIT_H

#include <stdlib.h>
#include <string.h>

extern int g_ctor;
extern int g_copy;
extern int g_dtor;

struct T {
    int v;
    T() { v = 1; g_ctor++; }
    T(const T &o) { v = o.v + 100; g_copy++; }
    ~T() { g_dtor++; }
};

T make_t(void);

// Copy-initialization written in the included file.
static int header_copy_init(int *copies)
{
    int before = g_copy;
    T b = make_t();
    *copies = g_copy - before;
    return b.v;
}

// Direct-initialization written in the included file (FEAT-4B, same gate).
static int header_direct_init(int *copies)
{
    int before;
    T a;
    before = g_copy;
    {
        T c(a);
        *copies = g_copy - before;
        return c.v;
    }
}

// ---------------------------------------------------------------------------
// Ownership probe.  This is the shape that actually broke amateras: win_txt
// owns a malloc'd buffer, so a copy that aliases the pointer gets the buffer
// freed twice.  A plain int member cannot show that, hence a real heap owner.
//
// own_unregister() reports whether the pointer was live, so a double free is
// counted instead of corrupting the heap and killing the test process.
// ---------------------------------------------------------------------------

#define OWN_MAX 64
extern void *g_live[OWN_MAX];
extern int   g_live_n;
extern int   g_alloc;
extern int   g_free;
extern int   g_badfree;

void own_register(void *p);
int  own_unregister(void *p);

struct Owner {
    char *p;
    Owner()
    {
        p = (char *)malloc(8);
        memcpy(p, "abc", 4);
        own_register(p);
        g_alloc++;
    }
    Owner(const Owner &o)
    {
        p = (char *)malloc(8);
        memcpy(p, o.p, 8);
        own_register(p);
        g_alloc++;
    }
    ~Owner()
    {
        if (p) {
            if (own_unregister(p)) {
                free(p);
                g_free++;
            } else {
                g_badfree++;   /* already freed or never registered */
            }
            p = 0;
        }
    }
};

Owner make_owner(void);

// `Owner b = a;` must deep copy: the two objects must not share the buffer.
// Returns 0 on success.
static int header_owner_alias(void)
{
    Owner a;
    char *pa = a.p;
    {
        Owner b = a;
        if (b.p == 0) return 1;
        if (b.p == pa) return 2;                      /* aliased the source */
        if (memcmp(b.p, "abc", 4) != 0) return 3;     /* content lost */
    }
    if (memcmp(pa, "abc", 4) != 0) return 4;          /* source freed early */
    return 0;
}

// `Owner b = make_owner();` - the full amateras shape (value return + copy-init).
// Returns 0 on success.
static int header_owner_roundtrip(void)
{
    Owner b = make_owner();
    if (b.p == 0) return 1;
    if (memcmp(b.p, "abc", 4) != 0) return 2;
    return 0;
}

#endif
