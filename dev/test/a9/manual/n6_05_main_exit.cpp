// N6-05 G2: exit() skips automatic dtors but runs TLS finalize before CRT exit.
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

static void touch_tls() { (void)&tls; }

int main()
{
    Auto a;
    (void)a;
    touch_tls();
    exit(23);
    return 0;
}
