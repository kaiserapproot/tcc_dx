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
// The absolute counts are a tpp quirk (struct returns move by memcpy); what
// must hold is that an included file behaves exactly like the primary TU.
#ifndef HEADER_LOCAL_COPY_INIT_H
#define HEADER_LOCAL_COPY_INIT_H

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

#endif
