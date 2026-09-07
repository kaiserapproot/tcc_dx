// N6-06A: run_enter before global ctor; TLS ctor before global dtor (-run).
#include <stdio.h>

static volatile int g_global_ctor;
static volatile int g_global_dtor;
static volatile int g_tls_ctor;

static void touch_tls(void);

struct G {
    G() {
        ++g_global_ctor;
        touch_tls();
        printf("GLOBAL_CTOR\n");
        fflush(stdout);
    }
    ~G() {
        ++g_global_dtor;
        printf("GLOBAL_DTOR\n");
        fflush(stdout);
    }
};

G g;

struct Tls {
    Tls() {
        ++g_tls_ctor;
        printf("TLS_CTOR\n");
        fflush(stdout);
    }
    ~Tls() {
        printf("TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local Tls tls;

static void touch_tls(void) { (void)&tls; }

int main()
{
    printf("MAIN\n");
    fflush(stdout);
    return 0;
}
