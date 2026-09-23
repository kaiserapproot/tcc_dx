// Local class initialization must behave the same whether the function body
// sits in the primary TU or in an #include'd file.  The invariant checked here
// is the equality of the two, not an absolute count: tpp moves struct return
// values with a memcpy rather than a copy ctor, so the raw numbers are a tpp
// quirk.  What broke real code (amateras cross.h win_txt) was that the
// included path skipped the copy ctor entirely and aliased the temporary's
// heap pointer, which was then freed twice.
//
// Pre-fix (a3eba99): direct-init in the header failed to compile with
// "lvalue expected"; with that case removed, copy-init gave header=101/1 copy
// against primary=201/2 copies.
#include <stdio.h>
#include "header_local_copy_init.h"

int g_ctor = 0;
int g_copy = 0;
int g_dtor = 0;

T make_t(void)
{
    T t;
    return t;
}

// Same bodies, in the primary TU.  These have always worked; they are the
// reference the included versions must match.
static int main_copy_init(int *copies)
{
    int before = g_copy;
    T b = make_t();
    *copies = g_copy - before;
    return b.v;
}

static int main_direct_init(int *copies)
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

int main(void)
{
    int fail = 0;
    int hc_v, mc_v, hd_v, md_v;
    int hc_n, mc_n, hd_n, md_n;

    hc_v = header_copy_init(&hc_n);
    mc_v = main_copy_init(&mc_n);
    hd_v = header_direct_init(&hd_n);
    md_v = main_direct_init(&md_n);

    printf("copy-init   header v=%d copies=%d | primary v=%d copies=%d\n",
           hc_v, hc_n, mc_v, mc_n);
    printf("direct-init header v=%d copies=%d | primary v=%d copies=%d\n",
           hd_v, hd_n, md_v, md_n);
    printf("ctor=%d copy=%d dtor=%d\n", g_ctor, g_copy, g_dtor);

    if (hc_v != mc_v || hc_n != mc_n) {
        printf("FAIL: copy-init differs between included file and primary TU\n");
        fail = 1;
    }
    if (hd_v != md_v || hd_n != md_n) {
        printf("FAIL: direct-init differs between included file and primary TU\n");
        fail = 1;
    }
    // The copy ctor must have run at least once on both paths; a raw memcpy
    // would leave the value at 1 and the count at 0.
    if (hc_n < 1 || hd_n < 1) {
        printf("FAIL: copy ctor never ran in the included file\n");
        fail = 1;
    }

    printf(fail ? "RESULT=FAIL\n" : "RESULT=PASS\n");
    return fail;
}
