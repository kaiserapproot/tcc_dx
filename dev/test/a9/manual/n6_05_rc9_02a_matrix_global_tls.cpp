// RC9-02A matrix: global=YES TLS=YES worker exit(9).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

static volatile int g_global_ctor;
static volatile int g_global_dtor;

struct G {
    G() { ++g_global_ctor; }
    ~G() {
        ++g_global_dtor;
        printf("GLOBAL_DTOR\n");
        fflush(stdout);
    }
};

G g;

struct TlsProbe {
    TlsProbe() { v = 1; }
    ~TlsProbe() { }
    int v;
};
thread_local TlsProbe tls_probe;

static void touch_tls(void)
{
    (void)tls_probe.v;
}

static DWORD WINAPI worker(void *p)
{
    (void)p;
    touch_tls();
    exit(9);
    return 0;
}

int main()
{
    HANDLE h;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    if (g_global_ctor != 1)
        return 1;
    touch_tls();
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h)
        return 1;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    printf("GLOBAL_CTOR_COUNT=%d\n", g_global_ctor);
    printf("GLOBAL_DTOR_COUNT=%d\n", g_global_dtor);
    fflush(stdout);
    return 0;
}
