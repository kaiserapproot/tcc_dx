// N6-06A2: PE DLL_THREAD_DETACH must NOOP after wrapper cleanup (no double drain/reclaim).
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

struct Tls {
    Tls() {}
    ~Tls() {}
};
thread_local Tls tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    (void)&tls;
    return 0;
}

int main(void)
{
    long before[ST_COUNT];
    long after[ST_COUNT];
    HANDLE h;
    long hook_d, drain_d, dtor_d, reclaim_d;

    __tcc_cpp_tls_n6_stats(before, ST_COUNT);
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    __tcc_cpp_tls_n6_stats(after, ST_COUNT);
    hook_d = after[ST_HOOK_DELIVERED] - before[ST_HOOK_DELIVERED];
    drain_d = after[ST_DRAIN_COMPLETED] - before[ST_DRAIN_COMPLETED];
    dtor_d = after[ST_DTOR_CALLS] - before[ST_DTOR_CALLS];
    reclaim_d = after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED];

    printf("PE_DETACH_AFTER_WRAPPER_CLEANUP=PASS\n");
    printf("PE_DETACH_SECOND_DTOR_COUNT=%ld\n", dtor_d > 1 ? dtor_d - 1 : 0);
    printf("PE_DETACH_SECOND_RECLAIM_COUNT=%ld\n", reclaim_d > 1 ? reclaim_d - 1 : 0);
    printf("HOOK_DELIVERED_DELTA=%ld DRAIN_COMPLETED_DELTA=%ld\n", hook_d, drain_d);
    printf("DOUBLE_DTOR=%s\n", dtor_d > 1 ? "YES" : "0");
    printf("DOUBLE_RECLAIM=%s\n", reclaim_d > 1 ? "YES" : "0");
    fflush(stdout);
    if (dtor_d != 1 || drain_d != 1 || reclaim_d != 1)
        return 2;
    if (dtor_d > 1 || reclaim_d > 1)
        return 3;
    return 0;
}
