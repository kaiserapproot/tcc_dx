// N6-06A: worker join + explicit __tcc_cpp_tls_n6_cleanup_current_thread (not automatic thread-exit).
#include <stdio.h>
#include <windows.h>

extern "C" int __cdecl __tcc_cpp_tls_n6_cleanup_current_thread(void);

static volatile int g_worker_tls_dtor;

struct Tls {
    Tls() {}
    ~Tls() {
        ++g_worker_tls_dtor;
        printf("WORKER_TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local Tls tls;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    (void)&tls;
    __tcc_cpp_tls_n6_cleanup_current_thread();
    return 0;
}

int main()
{
    HANDLE h;
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) {
        printf("CreateThread failed\n");
        return 1;
    }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    if (g_worker_tls_dtor != 1) {
        printf("WORKER_TLS_DTOR_COUNT=%d\n", (int)g_worker_tls_dtor);
        return 2;
    }
    printf("WORKER_EXPLICIT_TLS_CLEANUP=PASS\n");
    fflush(stdout);
    return 0;
}
