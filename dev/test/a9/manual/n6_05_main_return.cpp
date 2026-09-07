// N6-05 G1: main return runs TLS finalize before static/atexit; exit code preserved.
#include <stdio.h>

static volatile int g_tls_dtor;

struct Tls {
    Tls() {}
    ~Tls() { ++g_tls_dtor; printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

int main()
{
    (void)&tls;
    return 17;
}
