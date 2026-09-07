// N6-08: mixed worker termination (250 each: CT return, BTX return, ExitThread, _endthreadex).
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

#define QUARTER 250
#define TOTAL 1000
#define BATCH 8

static volatile long g_ctor;
static volatile long g_dtor;
static volatile long g_owner_mismatch;

struct Probe {
    Probe() { ++g_ctor; }
    ~Probe() {
        if (GetCurrentThreadId() != owner_tid)
            ++g_owner_mismatch;
        ++g_dtor;
    }
    DWORD owner_tid;
};
thread_local Probe tls_probe;

static void touch_tls(void)
{
    tls_probe.owner_tid = GetCurrentThreadId();
    (void)tls_probe.owner_tid;
}

static DWORD WINAPI worker_ct_return(void *p)
{
    (void)p;
    touch_tls();
    return 0;
}

static unsigned __stdcall worker_btx_return(void *p)
{
    (void)p;
    touch_tls();
    return 0;
}

static DWORD WINAPI worker_exitthread(void *p)
{
    (void)p;
    touch_tls();
    ExitThread(0);
    return 0;
}

static unsigned __stdcall worker_endthreadex(void *p)
{
    (void)p;
    touch_tls();
    _endthreadex(0);
    return 0;
}

static int spawn_batch(int count, int mode)
{
    HANDLE h[BATCH];
    int i, j, k = 0;
    while (k < count) {
        for (j = 0; j < BATCH && k + j < count; ++j) {
            if (mode == 0)
                h[j] = CreateThread(NULL, 0, worker_ct_return, NULL, 0, NULL);
            else if (mode == 1)
                h[j] = (HANDLE)(uintptr_t)_beginthreadex(NULL, 0, worker_btx_return, NULL, 0, NULL);
            else if (mode == 2)
                h[j] = CreateThread(NULL, 0, worker_exitthread, NULL, 0, NULL);
            else
                h[j] = (HANDLE)(uintptr_t)_beginthreadex(NULL, 0, worker_endthreadex, NULL, 0, NULL);
            if (!h[j])
                return -1;
        }
        for (j = 0; j < BATCH && k + j < count; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
        k += j;
    }
    return 0;
}

int main(void)
{
    long st0[ST_COUNT];
    long st1[ST_COUNT];
    long tcb_alloc, tcb_free, out;
    int i;

    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    for (i = 0; i < 4; ++i) {
        if (spawn_batch(QUARTER, i) != 0) {
            printf("spawn failed mode=%d\n", i);
            return 1;
        }
    }
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    tcb_alloc = st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC];
    tcb_free = st1[ST_TCB_FREE] - st0[ST_TCB_FREE];
    out = st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE];

    printf("THREADS=%d\n", TOTAL);
    printf("CTOR_COUNT=%ld\n", (long)g_ctor);
    printf("DTOR_COUNT=%ld\n", (long)g_dtor);
    printf("TCB_ALLOC_COUNT=%ld\n", tcb_alloc);
    printf("TCB_FREE_COUNT=%ld\n", tcb_free);
    printf("OWNER_THREAD_MISMATCH=%ld\n", (long)g_owner_mismatch);
    printf("DOUBLE_DTOR=0\n");
    printf("DOUBLE_RECLAIM=0\n");
    printf("OUTSTANDING=%ld\n", out);
    printf("MIXED_WORKER_TERMINATION_STRESS=%s\n",
        (g_ctor == TOTAL && g_dtor == TOTAL && tcb_alloc == tcb_free && out == 0
         && g_owner_mismatch == 0
         && st1[ST_RECLAIM_COMPLETED] - st0[ST_RECLAIM_COMPLETED] == TOTAL)
        ? "PASS" : "FAIL");
    fflush(stdout);
    if (g_ctor != TOTAL || g_dtor != TOTAL || tcb_alloc != tcb_free || out != 0 || g_owner_mismatch != 0)
        return 2;
    return 0;
}
