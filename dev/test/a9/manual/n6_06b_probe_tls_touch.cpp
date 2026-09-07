// N6-06B-00: thread_local touch for direct relocate manual call probes.
#include <stdio.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
void *__cdecl __tcc_cpp_tls_n6_current_tcb(void);
}

enum {
    ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE,
    ST_OBJECT_ALLOC, ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE,
    ST_HOOK_DELIVERED, ST_DRAIN_STARTED, ST_DRAIN_COMPLETED,
    ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
    ST_POST_FINALIZE_OBJECT, ST_COUNT
};

static volatile int g_tls_ctor;
static volatile int g_tls_dtor;

struct TlsObj {
    TlsObj() { ++g_tls_ctor; }
    ~TlsObj() { ++g_tls_dtor; }
};
thread_local TlsObj tls;

extern "C" int touch_tls(void)
{
    printf("COMPILED_FUNCTION_ENTRY\n");
    fflush(stdout);
    (void)&tls;
    printf("COMPILED_FUNCTION_RETURN\n");
    fflush(stdout);
    return 7;
}

extern "C" int probe_get_tls_counters(int *ctor, int *dtor, long *outstanding,
                                      long *dtor_calls, void **tcb)
{
    long st[ST_COUNT];
    __tcc_cpp_tls_n6_stats(st, ST_COUNT);
    if (ctor)
        *ctor = g_tls_ctor;
    if (dtor)
        *dtor = g_tls_dtor;
    if (outstanding)
        *outstanding = (st[ST_TCB_ALLOC] - st[ST_TCB_FREE])
            + (st[ST_OBJECT_ALLOC] - st[ST_OBJECT_FREE])
            + (st[ST_ENTRIES_ALLOC] - st[ST_ENTRIES_FREE])
            + (st[ST_DTORS_ALLOC] - st[ST_DTORS_FREE]);
    if (dtor_calls)
        *dtor_calls = st[ST_DTOR_CALLS];
    if (tcb)
        *tcb = __tcc_cpp_tls_n6_current_tcb();
    return 0;
}
