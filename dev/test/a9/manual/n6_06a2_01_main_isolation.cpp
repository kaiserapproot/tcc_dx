// N6-06A2-01: main TLS survives worker join; main must not drain worker TLS.
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
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
    ST_POST_FINALIZE_OBJECT, ST_COUNT
};

static volatile int g_main_dtor;
static volatile int g_worker_dtor;
static volatile DWORD g_worker_tid;
static volatile DWORD g_worker_dtor_tid;
static void *volatile g_worker_tcb;

struct MainTls {
    MainTls() { printf("MAIN_TLS_CTOR\n"); fflush(stdout); }
    ~MainTls() {
        ++g_main_dtor;
        printf("MAIN_TLS_DTOR tid=%lu\n", (unsigned long)GetCurrentThreadId());
        fflush(stdout);
    }
    int tag;
};
thread_local MainTls main_tls;

struct WorkerTls {
    WorkerTls() { printf("WORKER_TLS_CTOR tid=%lu\n", (unsigned long)GetCurrentThreadId()); fflush(stdout); }
    ~WorkerTls() {
        g_worker_dtor_tid = GetCurrentThreadId();
        ++g_worker_dtor;
        printf("WORKER_TLS_DTOR tid=%lu\n", (unsigned long)g_worker_dtor_tid);
        fflush(stdout);
    }
    int tag;
};
thread_local WorkerTls worker_tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    g_worker_tid = GetCurrentThreadId();
    printf("WORKER_ENTRY tid=%lu\n", (unsigned long)g_worker_tid);
    fflush(stdout);
    worker_tls.tag = 7;
    g_worker_tcb = __tcc_cpp_tls_n6_current_tcb();
    printf("WORKER_RETURN_BEGIN\n");
    fflush(stdout);
    return 0;
}

static void print_outstanding(long *st)
{
    long out;
    out = (st[ST_TCB_ALLOC] - st[ST_TCB_FREE])
        + (st[ST_OBJECT_ALLOC] - st[ST_OBJECT_FREE]);
    printf("OUTSTANDING=%ld\n", out);
    fflush(stdout);
}

int main(void)
{
    long before[ST_COUNT];
    long after_worker[ST_COUNT];
    long after_main_touch[ST_COUNT];
    HANDLE h;
    void *main_tcb_before;
    void *main_tcb_after;

    __tcc_cpp_tls_n6_stats(before, ST_COUNT);
    main_tls.tag = 3;
    main_tcb_before = __tcc_cpp_tls_n6_current_tcb();
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    printf("JOIN_RETURN\n");
    fflush(stdout);
    __tcc_cpp_tls_n6_stats(after_worker, ST_COUNT);
    main_tls.tag = 4;
    main_tcb_after = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_stats(after_main_touch, ST_COUNT);
    print_outstanding(after_main_touch);

    printf("MAIN_TLS_SURVIVES_WORKER_JOIN=%s\n",
           (g_main_dtor == 0 && main_tls.tag == 4) ? "PASS" : "FAIL");
    printf("MAIN_DRAINS_WORKER_TLS=%s\n",
           (main_tcb_before == main_tcb_after && main_tcb_after != 0) ? "NO" : "YES");
    printf("WORKER_TLS_DTOR_BEFORE_JOIN_RETURN=%s\n",
           (g_worker_dtor >= 1) ? "PASS" : "FAIL");
    printf("WORKER_TLS_DTOR_OWNER_THREAD=%s\n",
           (g_worker_dtor == 0 || g_worker_dtor_tid == g_worker_tid) ? "PASS" : "FAIL");
    printf("WORKER_DTOR_DELTA=%ld\n",
           after_worker[ST_DTOR_CALLS] - before[ST_DTOR_CALLS]);
    printf("CROSS_THREAD_DRAIN_COUNT=%ld\n",
           after_worker[ST_CROSS_THREAD_RECLAIM_SKIPPED] - before[ST_CROSS_THREAD_RECLAIM_SKIPPED]);
    fflush(stdout);
    if (g_main_dtor != 0 || main_tls.tag != 4)
        return 2;
    if (main_tcb_before != main_tcb_after || !main_tcb_after)
        return 3;
    if (g_worker_dtor < 1)
        return 4;
    if (g_worker_dtor_tid != g_worker_tid)
        return 5;
    if (after_worker[ST_CROSS_THREAD_RECLAIM_SKIPPED] != before[ST_CROSS_THREAD_RECLAIM_SKIPPED])
        return 6;
    return 0;
}
