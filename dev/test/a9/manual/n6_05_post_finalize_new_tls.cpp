// N6-05 G3: after main finalize, new main-thread TLS initialization is fail-closed.
#include <stdio.h>
#include <stdlib.h>

extern "C" unsigned __tcc_cpp_tls_n6_stats(long *out, unsigned max);

enum { ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE, ST_OBJECT_ALLOC,
       ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE, ST_HOOK_DELIVERED, ST_DRAIN_STARTED,
       ST_DRAIN_COMPLETED, ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
       ST_POST_FINALIZE_OBJECT, ST_COUNT };

struct Y {
    Y() { printf("Y_CTOR\n"); fflush(stdout); }
    ~Y() { printf("Y_DTOR\n"); fflush(stdout); }
};
thread_local Y y;

static void touch_y() { (void)&y; }

static void gate_cb()
{
    touch_y();
    printf("GATE_DTOR\n");
    fflush(stdout);
}

int main()
{
    if (atexit(gate_cb) != 0)
        return 1;
    return 0;
}
