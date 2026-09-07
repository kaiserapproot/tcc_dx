// N6-05 G4: main TLS reclaim completes (OUTSTANDING=0) before atexit callbacks.
#include <stdio.h>
#include <stdlib.h>

extern "C" unsigned __tcc_cpp_tls_n6_stats(long *out, unsigned max);

enum { ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE, ST_OBJECT_ALLOC,
       ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE, ST_HOOK_DELIVERED, ST_DRAIN_STARTED,
       ST_DRAIN_COMPLETED, ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
       ST_POST_FINALIZE_OBJECT, ST_COUNT };

struct Tls {
    Tls() {}
    ~Tls() { printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

static void inspect_at_atexit()
{
    long st[ST_COUNT];
    long outstanding;
    __tcc_cpp_tls_n6_stats(st, ST_COUNT);
    outstanding = (st[ST_TCB_ALLOC] - st[ST_TCB_FREE])
                + (st[ST_OBJECT_ALLOC] - st[ST_OBJECT_FREE])
                + (st[ST_ENTRIES_ALLOC] - st[ST_ENTRIES_FREE])
                + (st[ST_DTORS_ALLOC] - st[ST_DTORS_FREE]);
    printf("OUTSTANDING=%ld\n", outstanding);
    fflush(stdout);
    if (outstanding != 0)
        exit(91);
}

int main()
{
    (void)&tls;
    if (atexit(inspect_at_atexit) != 0)
        return 1;
    return 0;
}
