// Innermost header of the include-boundary test (included by
// include_boundary_outer.h, which the .cpp includes).  Two levels deep, so the
// gate has to accept a nested #include, not just a direct one.
#ifndef INCLUDE_BOUNDARY_INNER_H
#define INCLUDE_BOUNDARY_INNER_H

extern int g_inner_copies;

struct B {
    int v;
    B() { v = 3; }
    B(const B &o) { v = o.v + 10; g_inner_copies++; }
    ~B() { }
};

B make_b(void);

// Local copy-init, two includes deep.
static int inner_local_copy_init(void)
{
    B b = make_b();
    return b.v;
}

// Local direct-init, two includes deep.
static int inner_local_direct_init(void)
{
    B a;
    B c(a);
    return c.v;
}

// Namespace-scope dynamic copy-init from a nested header.  This is the
// N7-CROSS-01C path (cpp_global_copy_init_decl_allowed).  Having it in the
// same header as the local forms is the point of this test: both gates must
// classify this file the same way.
extern B g_inner_global;

#endif
