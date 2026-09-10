// RC9-02A matrix: global=NO TLS=YES worker exit(9).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

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
    touch_tls();
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h)
        return 1;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    return 0;
}
