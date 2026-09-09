// N6-05-GLOBAL-01D: global ctor, no TLS, main return 0.
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

static void report_counts(void)
{
    printf("GLOBAL_CTOR_COUNT=%d\n", g_global_ctor);
    printf("GLOBAL_DTOR_COUNT=%d\n", g_global_dtor);
    printf("TLS_CTOR_COUNT=%d\n", g_tls_ctor);
    printf("TLS_DTOR_COUNT=%d\n", g_tls_dtor);
    fflush(stdout);
}

int main()
{
    if (g_global_ctor != 1)
        return 1;
    if (atexit(report_counts) != 0)
        return 1;
    printf("MAIN\n");
    fflush(stdout);
    return 0;
}
