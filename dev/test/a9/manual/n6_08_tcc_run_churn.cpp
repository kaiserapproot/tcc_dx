// N6-08: tcc -run thread churn (500 CreateThread + 500 _beginthreadex).
#include <process.h>
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
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT
};

#define HALF 500
#define TOTAL 1000
#define BATCH 8

static volatile long g_ctor;
static volatile long g_dtor;

struct Obj {
    Obj() { ++g_ctor; }
    ~Obj() { ++g_dtor; }
};
thread_local Obj tls_obj;

static DWORD WINAPI worker_ct(void *p)
{
    (void)p;
    (void)&tls_obj;
    return 0;
}

static unsigned __stdcall worker_btx(void *p)
{
    (void)p;
    (void)&tls_obj;
    return 0;
}

int main(void)
{
    long st0[ST_COUNT];
    long st1[ST_COUNT];
    HANDLE h[BATCH];
    int i, j, n;
    long tcb_alloc, tcb_free, out;

    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    for (n = 0; n < HALF; n += BATCH) {
        for (j = 0; j < BATCH && n + j < HALF; ++j) {
            h[j] = CreateThread(NULL, 0, worker_ct, NULL, 0, NULL);
            if (!h[j]) {
                printf("CreateThread failed\n");
                return 1;
            }
        }
        for (j = 0; j < BATCH && n + j < HALF; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
    }
    for (n = 0; n < HALF; n += BATCH) {
        for (j = 0; j < BATCH && n + j < HALF; ++j) {
            h[j] = (HANDLE)(uintptr_t)_beginthreadex(NULL, 0, worker_btx, NULL, 0, NULL);
            if (!h[j]) {
                printf("_beginthreadex failed\n");
                return 1;
            }
        }
        for (j = 0; j < BATCH && n + j < HALF; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
    }
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    tcb_alloc = st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC];
    tcb_free = st1[ST_TCB_FREE] - st0[ST_TCB_FREE];
    out = st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE];

    printf("THREADS=%d\n", TOTAL);
    printf("CREATE_THREAD_COUNT=%d\n", HALF);
    printf("BEGIN_THREADEX_COUNT=%d\n", HALF);
    printf("TLS_CTOR_COUNT=%ld\n", (long)g_ctor);
    printf("TLS_DTOR_COUNT=%ld\n", (long)g_dtor);
    printf("TCB_ALLOC_COUNT=%ld\n", tcb_alloc);
    printf("TCB_FREE_COUNT=%ld\n", tcb_free);
    printf("OUTSTANDING=%ld\n", out);
    printf("TCC_RUN_THREAD_CHURN=%s\n",
        (g_ctor == TOTAL && g_dtor == TOTAL && tcb_alloc == tcb_free && out == 0
         && st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED] == 0)
        ? "PASS" : "FAIL");
    fflush(stdout);
    if (g_ctor != TOTAL || g_dtor != TOTAL || tcb_alloc != tcb_free || out != 0)
        return 2;
    return 0;
}
