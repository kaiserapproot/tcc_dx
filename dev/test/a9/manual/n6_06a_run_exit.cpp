// N6-06A: exit() skips automatic dtors but runs TLS finalize before runmain unwind.
#include <stdio.h>
#include <stdlib.h>

static volatile int g_auto_dtor;
static volatile int g_tls_dtor;

struct Auto {
    Auto() {}
    ~Auto() { ++g_auto_dtor; printf("AUTO_DTOR\n"); fflush(stdout); }
};
struct Tls {
    Tls() {}
    ~Tls() { ++g_tls_dtor; printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

int main()
{
    Auto a;
    (void)a;
    (void)&tls;
    exit(23);
    return 0;
}
