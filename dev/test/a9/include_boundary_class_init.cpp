// Boundary test for the widened cpp_allow_local_class_direct_init() gate.
//
// The gate now says "real primary TU and real #include files; synthetic replay
// files only through cpp_user_inline_feat4b_replay".  That is the same
// classification cpp_global_copy_init_decl_allowed() has used since
// N7-CROSS-01C, so this file exercises both gates from the same headers and
// requires them to agree:
//
//   depth 0 (this file)                   local copy-init / direct-init
//   depth 1 (include_boundary_outer.h)    local copy-init / direct-init
//   depth 2 (include_boundary_inner.h)    local copy-init / direct-init
//                                         + namespace-scope dynamic copy-init
//
// A system header cannot host a class with a constructor, so the system-header
// side of the boundary is covered by the compile of <stdio.h> below plus the
// windows.h cases in dev/test/a9/manual/bug52_53_measure.bat.
//
// Every level must produce the same values.  Pre-fix, depths 1 and 2 silently
// took the C struct-assignment path (and the direct-init form did not compile
// at all), while depth 0 used the C++ path.
#include <stdio.h>
#include "include_boundary_outer.h"

int g_inner_copies = 0;

B make_b(void)
{
    B b;
    return b;
}

// Namespace-scope dynamic copy-init declared in the nested header.
B g_inner_global = make_b();

static int primary_local_copy_init(void)
{
    B b = make_b();
    return b.v;
}

static int primary_local_direct_init(void)
{
    B a;
    B c(a);
    return c.v;
}

int main(void)
{
    int fail = 0;
    int p_copy, o_copy, i_copy;
    int p_direct, o_direct, i_direct;

    p_copy = primary_local_copy_init();
    o_copy = outer_local_copy_init();
    i_copy = inner_local_copy_init();

    p_direct = primary_local_direct_init();
    o_direct = outer_local_direct_init();
    i_direct = inner_local_direct_init();

    printf("copy-init   primary=%d outer=%d inner=%d\n", p_copy, o_copy, i_copy);
    printf("direct-init primary=%d outer=%d inner=%d\n", p_direct, o_direct, i_direct);
    printf("global copy-init v=%d (want same as local copy-init)\n", g_inner_global.v);
    printf("inner copies counted = %d\n", g_inner_copies);

    if (p_copy != o_copy || p_copy != i_copy) {
        printf("FAIL: local copy-init differs by include depth\n");
        fail = 1;
    }
    if (p_direct != o_direct || p_direct != i_direct) {
        printf("FAIL: local direct-init differs by include depth\n");
        fail = 1;
    }
    // Direct-init runs the copy ctor exactly once: 3 + 10.
    if (p_direct != 13) {
        printf("FAIL: direct-init did not run the copy ctor (v=%d)\n", p_direct);
        fail = 1;
    }
    // The global was copy-initialized from make_b() in a nested header, which
    // is the cpp_global_copy_init_decl_allowed() path.  It must land on the
    // same value as the local copy-init, because both gates are supposed to
    // classify this file the same way.  Pre-N7-CROSS-01C the global stayed at
    // the constant 3 (no dynamic init at all).
    if (g_inner_global.v != p_copy) {
        printf("FAIL: global gate and local gate disagree (global=%d local=%d)\n",
               g_inner_global.v, p_copy);
        fail = 1;
    }

    printf(fail ? "RESULT=FAIL\n" : "RESULT=PASS\n");
    return fail;
}
