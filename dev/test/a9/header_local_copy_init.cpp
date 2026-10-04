// Local class initialization must behave the same whether the function body
// sits in the primary TU or in an #include'd file, and a class that owns heap
// memory must never be copied by aliasing its pointer.
//
// The equality checks use T (an int member): the absolute copy counts are a
// tpp quirk because struct return values move by memcpy, so what is asserted
// is that the included path matches the primary path.
// The ownership checks use Owner (a malloc'd buffer): that is the shape that
// actually broke amateras cross.h's win_txt, where the skipped copy ctor left
// two objects sharing one buffer and it was freed twice.
//
// Pre-fix (a3eba99): direct-init in the header failed to compile with
// "lvalue expected"; with that case removed, copy-init gave header=101/1 copy
// against primary=201/2 copies and the ownership probe reported a bad free.
#include <stdio.h>
#include "header_local_copy_init.h"

int g_ctor = 0;
int g_copy = 0;
int g_dtor = 0;

void *g_live[OWN_MAX];
int   g_live_n = 0;
int   g_alloc = 0;
int   g_free = 0;
int   g_badfree = 0;

void own_register(void *p)
{
    if (g_live_n < OWN_MAX) g_live[g_live_n++] = p;
}

int own_unregister(void *p)
{
    int i;
    for (i = 0; i < g_live_n; i++) {
        if (g_live[i] == p) {
            g_live[i] = g_live[g_live_n - 1];
            g_live_n--;
            return 1;
        }
    }
    return 0;
}

T make_t(void)
{
    T t;
    return t;
}

Owner make_owner(void)
{
    Owner o;
    return o;
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

static int main_owner_alias(void)
{
    Owner a;
    char *pa = a.p;
    {
        Owner b = a;
        if (b.p == 0) return 1;
        if (b.p == pa) return 2;
        if (memcmp(b.p, "abc", 4) != 0) return 3;
    }
    if (memcmp(pa, "abc", 4) != 0) return 4;
    return 0;
}

static int main_owner_roundtrip(void)
{
    Owner b = make_owner();
    if (b.p == 0) return 1;
    if (memcmp(b.p, "abc", 4) != 0) return 2;
    return 0;
}

int main(void)
{
    int fail = 0;
    int hc_v, mc_v, hd_v, md_v;
    int hc_n, mc_n, hd_n, md_n;
    int ha, ma, hr, mr;

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
    if (hc_n < 1 || hd_n < 1) {
        printf("FAIL: copy ctor never ran in the included file\n");
        fail = 1;
    }

    ha = header_owner_alias();
    ma = main_owner_alias();
    hr = header_owner_roundtrip();
    mr = main_owner_roundtrip();

    printf("owner alias      header=%d primary=%d (0 = deep copy)\n", ha, ma);
    printf("owner roundtrip  header=%d primary=%d (0 = value survived)\n", hr, mr);
    printf("alloc=%d free=%d badfree=%d live=%d\n",
           g_alloc, g_free, g_badfree, g_live_n);

    if (ha != 0) { printf("FAIL: header owner alias check = %d\n", ha); fail = 1; }
    if (ma != 0) { printf("FAIL: primary owner alias check = %d\n", ma); fail = 1; }
    if (hr != 0) { printf("FAIL: header owner roundtrip = %d\n", hr); fail = 1; }
    if (mr != 0) { printf("FAIL: primary owner roundtrip = %d\n", mr); fail = 1; }

    // The real authority for the amateras defect: every buffer freed exactly
    // once, nothing freed twice, nothing left over.
    if (g_badfree != 0) {
        printf("FAIL: %d double / invalid free(s)\n", g_badfree);
        fail = 1;
    }
    if (g_alloc != g_free || g_live_n != 0) {
        printf("FAIL: alloc %d != free %d (live %d)\n", g_alloc, g_free, g_live_n);
        fail = 1;
    }

    printf(fail ? "RESULT=FAIL\n" : "RESULT=PASS\n");
    return fail;
}
