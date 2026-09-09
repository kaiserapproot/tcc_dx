// N6-05-GLOBAL-01D: global ctor, no TLS, main thread exit(7).
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

int main()
{
    if (g_global_ctor != 1)
        return 1;
    printf("MAIN\n");
    fflush(stdout);
    exit(7);
    return 0;
}
