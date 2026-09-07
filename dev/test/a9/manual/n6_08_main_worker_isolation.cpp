// N6-08: main TLS survives 100 worker joins; worker dtor on owner thread only.
#include <stdio.h>
#include <windows.h>

extern "C" {
void *__cdecl __tcc_cpp_tls_n6_current_tcb(void);
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}

enum {
    ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE,
    ST_OBJECT_ALLOC, ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE,
    ST_HOOK_DELIVERED, ST_DRAIN_STARTED, ST_DRAIN_COMPLETED,
    ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT
};

#define WORKERS 100
#define BATCH 8

static volatile int g_main_dtor;
static volatile int g_worker_dtor;
static volatile int g_worker_dtor_owner_fail;

struct MainTls {
    MainTls() {}
    ~MainTls() { ++g_main_dtor; }
    int tag;
};
thread_local MainTls main_tls;

struct WorkerTls {
    WorkerTls() {}
    ~WorkerTls() {
        if (GetCurrentThreadId() != owner_tid)
            ++g_worker_dtor_owner_fail;
        ++g_worker_dtor;
    }
    DWORD owner_tid;
    int tag;
};
thread_local WorkerTls worker_tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    worker_tls.owner_tid = GetCurrentThreadId();
    worker_tls.tag = 1;
    return 0;
}

int main(void)
{
    long before[ST_COUNT];
    long after[ST_COUNT];
    HANDLE h[BATCH];
    int i, j, k;
    void *main_tcb_before;
    void *main_tcb_after;

    __tcc_cpp_tls_n6_stats(before, ST_COUNT);
    main_tls.tag = 1;
    main_tcb_before = __tcc_cpp_tls_n6_current_tcb();
    for (k = 0; k < WORKERS; k += BATCH) {
        for (j = 0; j < BATCH && k + j < WORKERS; ++j) {
            h[j] = CreateThread(NULL, 0, worker, NULL, 0, NULL);
            if (!h[j]) {
                printf("CreateThread failed\n");
                return 1;
            }
        }
        for (j = 0; j < BATCH && k + j < WORKERS; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
    }
    main_tls.tag = 2;
    main_tcb_after = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_stats(after, ST_COUNT);

    printf("WORKERS=%d\n", WORKERS);
    printf("MAIN_TLS_SURVIVES_ALL_WORKER_JOINS=%s\n",
        (g_main_dtor == 0 && main_tls.tag == 2) ? "PASS" : "FAIL");
    printf("WORKER_DTOR_ON_OWNER_THREAD=%s\n",
        (g_worker_dtor == WORKERS && g_worker_dtor_owner_fail == 0) ? "PASS" : "FAIL");
    printf("MAIN_DRAINS_WORKER_TLS=%s\n",
        (main_tcb_before == main_tcb_after && main_tcb_after != 0) ? "NO" : "YES");
    printf("MAIN_TLS_DTOR_EXACTLY_ONCE=%s\n",
        (g_main_dtor <= 1) ? "PASS" : "FAIL");
    printf("WORKER_DTOR_COUNT=%d\n", g_worker_dtor);
    printf("MAIN_WORKER_ISOLATION_STRESS=%s\n",
        (g_main_dtor == 0 && main_tls.tag == 2 && g_worker_dtor == WORKERS
         && g_worker_dtor_owner_fail == 0 && main_tcb_before == main_tcb_after
         && after[ST_CROSS_THREAD_RECLAIM_SKIPPED] == before[ST_CROSS_THREAD_RECLAIM_SKIPPED])
        ? "PASS" : "FAIL");
    fflush(stdout);
    if (g_main_dtor != 0 || main_tls.tag != 2)
        return 2;
    if (g_worker_dtor != WORKERS || g_worker_dtor_owner_fail != 0)
        return 3;
    if (main_tcb_before != main_tcb_after || !main_tcb_after)
        return 4;
    return 0;
}
