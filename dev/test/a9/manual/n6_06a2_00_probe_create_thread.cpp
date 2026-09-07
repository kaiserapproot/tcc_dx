// N6-06A2-00: -run worker via CreateThread, normal return, NO explicit TLS cleanup.
#include <stdio.h>
#include <windows.h>

extern "C" {
void *__cdecl __tcc_cpp_tls_n6_current_tcb(void);
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}

enum {
    ST_TCB_ALLOC, ST_TCB_FREE, ST_OBJECT_ALLOC, ST_OBJECT_FREE,
    ST_HOOK_DELIVERED, ST_DRAIN_STARTED, ST_DRAIN_COMPLETED,
    ST_RECLAIM_COMPLETED, ST_DTOR_CALLS, ST_COUNT
};

static volatile int g_worker_dtor;
static volatile int g_worker_touched;

struct Tls {
    Tls() {}
    ~Tls() {
        ++g_worker_dtor;
        printf("WORKER_TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local Tls tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    (void)&tls;
    g_worker_touched = 1;
    return 0;
}

static void snapshot(long *out)
{
    __tcc_cpp_tls_n6_stats(out, ST_COUNT);
}

int main(void)
{
    long before[ST_COUNT];
    long after[ST_COUNT];
    HANDLE h;
    void *worker_tcb;

    snapshot(before);
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    snapshot(after);
    worker_tcb = __tcc_cpp_tls_n6_current_tcb();

    printf("PATH=CREATE_THREAD_NORMAL_RETURN\n");
    printf("WORKER_TOUCHED_TLS=%s\n", g_worker_touched ? "YES" : "NO");
    printf("WORKER_CALLED_EXPLICIT_CLEANUP=NO\n");
    printf("WORKER_TLS_DTOR_OBSERVED=%s\n", g_worker_dtor ? "YES" : "NO");
    printf("HOOK_DELIVERED_DELTA=%ld\n", after[ST_HOOK_DELIVERED] - before[ST_HOOK_DELIVERED]);
    printf("DTOR_CALLS_DELTA=%ld\n", after[ST_DTOR_CALLS] - before[ST_DTOR_CALLS]);
    printf("TCB_ALLOC_DELTA=%ld\n", after[ST_TCB_ALLOC] - before[ST_TCB_ALLOC]);
    printf("TCB_FREE_DELTA=%ld\n", after[ST_TCB_FREE] - before[ST_TCB_FREE]);
    printf("RECLAIM_COMPLETED_DELTA=%ld\n", after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED]);
    printf("MAIN_CURRENT_TCB_AFTER_JOIN=%s\n", worker_tcb ? "NON_NULL" : "NULL");
    printf("MEASURE_USER_TLS_DTOR=%s\n", g_worker_dtor ? "YES" : "NO");
    printf("MEASURE_HOOK_DELIVERED=%s\n", (after[ST_HOOK_DELIVERED] - before[ST_HOOK_DELIVERED]) ? "YES" : "NO");
    printf("MEASURE_DTOR_CALLS=%s\n", (after[ST_DTOR_CALLS] - before[ST_DTOR_CALLS]) ? "YES" : "NO");
    printf("MEASURE_RECLAIM=%s\n", (after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED]) ? "YES" : "NO");
    fflush(stdout);
    return 0;
}
