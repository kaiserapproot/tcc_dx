// N6-05: fallthrough main (implicit return 0) uses the same TLS gateway as return.
#include <stdio.h>

static volatile int g_tls_dtor;

struct Tls {
    Tls() {}
    ~Tls() { ++g_tls_dtor; printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

static void touch_tls() { (void)&tls; }

int main()
{
    touch_tls();
}
