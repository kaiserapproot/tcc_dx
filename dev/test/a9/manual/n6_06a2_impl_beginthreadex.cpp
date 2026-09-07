// N6-06A2: _beginthreadex normal return with wrapper cleanup (-run).
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
    ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
    ST_POST_FINALIZE_OBJECT, ST_COUNT
};

static volatile int g_worker_dtor;
static volatile DWORD g_worker_tid;
static volatile DWORD g_dtor_tid;

struct Tls {
    Tls() {}
    ~Tls() {
        g_dtor_tid = GetCurrentThreadId();
        ++g_worker_dtor;
        printf("TLS_DTOR tid=%lu\n", (unsigned long)g_dtor_tid);
        fflush(stdout);
    }
};
thread_local Tls tls;

static unsigned __stdcall worker(void *p)
{
    (void)p;
    g_worker_tid = GetCurrentThreadId();
    (void)&tls;
    return 0;
}

int main(void)
{
    long before[ST_COUNT];
    long after[ST_COUNT];
    uintptr_t th;
    long out;

    __tcc_cpp_tls_n6_stats(before, ST_COUNT);
    th = _beginthreadex(NULL, 0, worker, NULL, 0, NULL);
    if (!th) {
        printf("_beginthreadex failed\n");
        return 1;
    }
    WaitForSingleObject((HANDLE)th, INFINITE);
    CloseHandle((HANDLE)th);
    __tcc_cpp_tls_n6_stats(after, ST_COUNT);
    out = (after[ST_TCB_ALLOC] - after[ST_TCB_FREE])
        + (after[ST_OBJECT_ALLOC] - after[ST_OBJECT_FREE]);

    printf("BEGINTHREADEX_NORMAL_RETURN_TLS_DTOR=%s\n", g_worker_dtor ? "PASS" : "FAIL");
    printf("BEGINTHREADEX_NORMAL_RETURN_TLS_RECLAIM=%s\n",
           (after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED]) >= 1
           && (after[ST_TCB_FREE] - before[ST_TCB_FREE]) >= 1 && out == 0 ? "PASS" : "FAIL");
    printf("WORKER_TLS_DTOR_OWNER_THREAD=%s\n",
           (g_worker_dtor == 0 || g_dtor_tid == g_worker_tid) ? "PASS" : "FAIL");
    printf("OUTSTANDING=%ld\n", out);
    fflush(stdout);
    if (!g_worker_dtor || g_dtor_tid != g_worker_tid)
        return 2;
    if (after[ST_DTOR_CALLS] - before[ST_DTOR_CALLS] < 1)
        return 3;
    if (after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED] < 1)
        return 4;
    if (out != 0)
        return 5;
    return 0;
}
