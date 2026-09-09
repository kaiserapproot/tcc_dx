// N6-05-GLOBAL-01D: C++ global init, no TLS, worker exit(9) fail-closed.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static volatile int g_global_ctor;
static volatile int g_global_dtor;
static volatile int g_tls_ctor;
static volatile int g_tls_dtor;

struct G {
    G() { ++g_global_ctor; }
    ~G() {
        ++g_global_dtor;
        printf("GLOBAL_DTOR\n");
        fflush(stdout);
    }
};

G g;

static DWORD WINAPI worker(void *p)
{
    (void)p;
    exit(9);
    return 0;
}

int main()
{
    HANDLE h;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    if (g_global_ctor != 1)
        return 1;
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h)
        return 1;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    printf("GLOBAL_CTOR_COUNT=%d\n", g_global_ctor);
    printf("GLOBAL_DTOR_COUNT=%d\n", g_global_dtor);
    printf("TLS_CTOR_COUNT=%d\n", g_tls_ctor);
    printf("TLS_DTOR_COUNT=%d\n", g_tls_dtor);
    fflush(stdout);
    return 0;
}
