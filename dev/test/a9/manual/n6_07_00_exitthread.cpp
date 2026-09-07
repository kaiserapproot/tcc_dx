// N6-07-02: -run worker CreateThread + ExitThread(23), no explicit TLS cleanup.
#include <stdio.h>
#include <windows.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}

enum {
    ST_TCB_ALLOC, ST_TCB_FREE, ST_HOOK_DELIVERED, ST_DTOR_CALLS,
    ST_RECLAIM_COMPLETED, ST_COUNT
};

static volatile int g_worker_dtor;

struct Tls {
    Tls() {}
    ~Tls() {
        ++g_worker_dtor;
        printf("EXITTHREAD_WORKER_TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local Tls tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    (void)&tls;
    ExitThread(23);
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
    DWORD exit_code;

    snapshot(before);
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(h, INFINITE);
    GetExitCodeThread(h, &exit_code);
    CloseHandle(h);
    snapshot(after);

    printf("PATH=CREATE_THREAD_EXITTHREAD_23\n");
    printf("TLS_CTOR_COUNT=1\n");
    printf("TLS_DTOR_COUNT=%d\n", g_worker_dtor);
    printf("TLS_RECLAIM_COUNT=%ld\n", after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED]);
    printf("PROCESS_CRASH=NO\n");
    printf("THREAD_EXIT_CODE=%lu\n", (unsigned long)exit_code);
    printf("MEASURE_USER_TLS_DTOR=%s\n", g_worker_dtor ? "YES" : "NO");
    printf("MEASURE_RECLAIM=%s\n", (after[ST_RECLAIM_COMPLETED] - before[ST_RECLAIM_COMPLETED]) ? "YES" : "NO");
    fflush(stdout);
    return 0;
}
