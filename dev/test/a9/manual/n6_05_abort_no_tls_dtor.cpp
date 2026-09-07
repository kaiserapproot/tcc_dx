// N6-05 K: abort() does not run TLS destructors on main thread.
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static volatile int g_tls_dtor;

struct Tls {
    Tls() {}
    ~Tls() { ++g_tls_dtor; printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

int main()
{
    // Suppress WER/fault UI so abort() terminates promptly in batch CI.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    (void)&tls;
    abort();
    return 0;
}
