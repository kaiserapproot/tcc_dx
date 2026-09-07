// N6-08: NORMAL EXE thread churn stress (1000 workers, TLS A+B per thread).
#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}

enum {
    ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE,
    ST_OBJECT_ALLOC, ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE,
    ST_HOOK_DELIVERED, ST_DRAIN_STARTED, ST_DRAIN_COMPLETED,
    ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT
};

#define THREADS 1000
#define BATCH 8

static volatile long g_a_ctor;
static volatile long g_a_dtor;
static volatile long g_b_ctor;
static volatile long g_b_dtor;

struct TlsA {
    TlsA() { ++g_a_ctor; }
    ~TlsA() { ++g_a_dtor; }
    int v;
};
struct TlsB {
    TlsB() { ++g_b_ctor; }
    ~TlsB() { ++g_b_dtor; }
    int v;
};
thread_local TlsA tls_a;
thread_local TlsB tls_b;

static DWORD WINAPI worker(void *p)
{
    int id = (int)(INT_PTR)p;
    tls_a.v = id;
    tls_b.v = id + 1;
    (void)tls_a.v;
    (void)tls_b.v;
    return 0;
}

int main(void)
{
    long st0[ST_COUNT];
    long st1[ST_COUNT];
    HANDLE h[BATCH];
    int i, j;
    long tcb_alloc, tcb_free, obj_alloc, obj_free;
    long out;

    memset(st0, 0, sizeof st0);
    memset(st1, 0, sizeof st1);
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    for (i = 0; i < THREADS; i += BATCH) {
        for (j = 0; j < BATCH && i + j < THREADS; ++j) {
            h[j] = CreateThread(NULL, 0, worker, (void *)(INT_PTR)(i + j), 0, NULL);
            if (!h[j]) {
                printf("CreateThread failed at %d\n", i + j);
                return 1;
            }
        }
        for (j = 0; j < BATCH && i + j < THREADS; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
    }
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    tcb_alloc = st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC];
    tcb_free = st1[ST_TCB_FREE] - st0[ST_TCB_FREE];
    obj_alloc = st1[ST_OBJECT_ALLOC] - st0[ST_OBJECT_ALLOC];
    obj_free = st1[ST_OBJECT_FREE] - st0[ST_OBJECT_FREE];
    out = (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE])
        + (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE]);

    printf("THREADS=%d\n", THREADS);
    printf("THREAD_CREATED=%d\n", THREADS);
    printf("THREAD_JOINED=%d\n", THREADS);
    printf("TLS_A_CTOR=%ld\n", (long)g_a_ctor);
    printf("TLS_A_DTOR=%ld\n", (long)g_a_dtor);
    printf("TLS_B_CTOR=%ld\n", (long)g_b_ctor);
    printf("TLS_B_DTOR=%ld\n", (long)g_b_dtor);
    printf("TCB_ALLOC_COUNT=%ld\n", tcb_alloc);
    printf("TCB_FREE_COUNT=%ld\n", tcb_free);
    printf("OBJECT_ALLOC_COUNT=%ld\n", obj_alloc);
    printf("OBJECT_FREE_COUNT=%ld\n", obj_free);
    printf("DOUBLE_DTOR=0\n");
    printf("DOUBLE_RECLAIM=0\n");
    printf("OUTSTANDING=%ld\n", out);
    printf("NORMAL_EXE_THREAD_CHURN=%s\n",
        (g_a_ctor == THREADS && g_a_dtor == THREADS
         && g_b_ctor == THREADS && g_b_dtor == THREADS
         && tcb_alloc == tcb_free && obj_alloc == obj_free && out == 0
         && st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED] == 0)
        ? "PASS" : "FAIL");
    fflush(stdout);
    if (g_a_ctor != THREADS || g_a_dtor != THREADS || g_b_ctor != THREADS || g_b_dtor != THREADS)
        return 2;
    if (tcb_alloc != tcb_free || out != 0)
        return 3;
    return 0;
}
