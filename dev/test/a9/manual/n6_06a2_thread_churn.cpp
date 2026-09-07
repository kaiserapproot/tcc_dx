// N6-06A2: 1000 worker threads via -run CreateThread wrapper, OUTSTANDING=0.
#include <stdio.h>
#include <windows.h>

extern "C" {
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

#define THREADS 1000
#define BATCH 8

static volatile int g_ctor;
static volatile int g_dtor;

struct Obj {
    Obj() { ++g_ctor; }
    ~Obj() { ++g_dtor; }
};
thread_local Obj obj;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    (void)&obj;
    return 0;
}

int main(void)
{
    long st0[ST_COUNT];
    long st1[ST_COUNT];
    HANDLE h[BATCH];
    int i, j;
    long alloc_total, free_total, out;

    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    for (i = 0; i < THREADS; i += BATCH) {
        for (j = 0; j < BATCH && i + j < THREADS; ++j) {
            h[j] = CreateThread(NULL, 0, worker, NULL, 0, NULL);
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
    alloc_total = (st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC])
        + (st1[ST_ENTRIES_ALLOC] - st0[ST_ENTRIES_ALLOC])
        + (st1[ST_OBJECT_ALLOC] - st0[ST_OBJECT_ALLOC])
        + (st1[ST_DTORS_ALLOC] - st0[ST_DTORS_ALLOC]);
    free_total = (st1[ST_TCB_FREE] - st0[ST_TCB_FREE])
        + (st1[ST_ENTRIES_FREE] - st0[ST_ENTRIES_FREE])
        + (st1[ST_OBJECT_FREE] - st0[ST_OBJECT_FREE])
        + (st1[ST_DTORS_FREE] - st0[ST_DTORS_FREE]);
    out = (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE])
        + (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE])
        + (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE])
        + (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]);

    printf("THREAD_COUNT=%d\n", THREADS);
    printf("TLS_CTOR_COUNT=%d\n", g_ctor);
    printf("TLS_DTOR_COUNT=%d\n", g_dtor);
    printf("TCB_ALLOC_COUNT=%ld\n", st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC]);
    printf("TCB_FREE_COUNT=%ld\n", st1[ST_TCB_FREE] - st0[ST_TCB_FREE]);
    printf("OUTSTANDING=%ld\n", out);
    printf("THREAD_CHURN=%s\n",
           (g_ctor == THREADS && g_dtor == THREADS && out == 0
            && alloc_total == free_total
            && st1[ST_DTOR_CALLS] - st0[ST_DTOR_CALLS] == THREADS
            && st1[ST_RECLAIM_COMPLETED] - st0[ST_RECLAIM_COMPLETED] == THREADS
            && st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED] == 0)
           ? "PASS" : "FAIL");
    fflush(stdout);
    if (g_ctor != THREADS || g_dtor != THREADS || out != 0 || alloc_total != free_total)
        return 2;
    return 0;
}
