// N6-06A2-01: two workers, independent TCB/TLS owners; main must not drain either.
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

static void *volatile g_tcb_a;
static void *volatile g_tcb_b;
static volatile DWORD g_tid_a;
static volatile DWORD g_tid_b;
static volatile DWORD g_dtor_tid_a;
static volatile DWORD g_dtor_tid_b;
static volatile int g_dtor_a;
static volatile int g_dtor_b;

struct SlotTls {
    int id;
    SlotTls() {}
    ~SlotTls() {
        DWORD tid = GetCurrentThreadId();
        if (id == 1) {
            g_dtor_tid_a = tid;
            ++g_dtor_a;
            printf("WORKER_A_TLS_DTOR tid=%lu\n", (unsigned long)tid);
        } else {
            g_dtor_tid_b = tid;
            ++g_dtor_b;
            printf("WORKER_B_TLS_DTOR tid=%lu\n", (unsigned long)tid);
        }
        fflush(stdout);
    }
};
thread_local SlotTls slot_tls;

static DWORD WINAPI worker(void *p)
{
    int id = (int)(INT_PTR)p;
    slot_tls.id = id;
    if (id == 1) {
        g_tid_a = GetCurrentThreadId();
        g_tcb_a = __tcc_cpp_tls_n6_current_tcb();
        printf("WORKER_A_ENTRY tid=%lu\n", (unsigned long)g_tid_a);
    } else {
        g_tid_b = GetCurrentThreadId();
        g_tcb_b = __tcc_cpp_tls_n6_current_tcb();
        printf("WORKER_B_ENTRY tid=%lu\n", (unsigned long)g_tid_b);
    }
    fflush(stdout);
    return 0;
}

int main(void)
{
    long st0[ST_COUNT];
    long st1[ST_COUNT];
    HANDLE ha, hb;

    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    ha = CreateThread(NULL, 0, worker, (void *)(INT_PTR)1, 0, NULL);
    hb = CreateThread(NULL, 0, worker, (void *)(INT_PTR)2, 0, NULL);
    if (!ha || !hb) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha);
    CloseHandle(hb);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);

    printf("WORKER_A_TCB=%p WORKER_B_TCB=%p\n", g_tcb_a, g_tcb_b);
    printf("MULTI_WORKER_TLS_ISOLATION=%s\n",
           (g_tcb_a && g_tcb_b && g_tcb_a != g_tcb_b) ? "PASS" : "FAIL");
    printf("WORKER_A_TLS_OWNER=%s\n",
           (g_dtor_a == 0 || g_dtor_tid_a == g_tid_a) ? "WORKER_A" : "FAIL");
    printf("WORKER_B_TLS_OWNER=%s\n",
           (g_dtor_b == 0 || g_dtor_tid_b == g_tid_b) ? "WORKER_B" : "FAIL");
    printf("MAIN_DRAINS_WORKER_TLS=%s\n",
           (g_dtor_a >= 1 && g_dtor_b >= 1) ? "NO" : "UNKNOWN");
    printf("CROSS_THREAD_DRAIN_COUNT=%ld\n",
           st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED]);
    fflush(stdout);

    if (!g_tcb_a || !g_tcb_b || g_tcb_a == g_tcb_b)
        return 2;
    if (g_dtor_a < 1 || g_dtor_b < 1)
        return 3;
    if (g_dtor_tid_a != g_tid_a || g_dtor_tid_b != g_tid_b)
        return 4;
    if (st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] != st0[ST_CROSS_THREAD_RECLAIM_SKIPPED])
        return 5;
    return 0;
}
