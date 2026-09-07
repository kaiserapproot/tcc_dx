// N6-06B-00: manual main + exit() in child process only.
#include <stdio.h>
#include <stdlib.h>

static volatile int g_tls_dtor;

struct ExitTls {
    ExitTls() {}
    ~ExitTls() {
        ++g_tls_dtor;
        printf("MANUAL_EXIT_TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local ExitTls exit_tls;

extern "C" int main(void)
{
    (void)&exit_tls;
    printf("MANUAL_MAIN_BEFORE_EXIT\n");
    fflush(stdout);
    exit(23);
    return 0;
}

extern "C" int probe_exit_tls_dtor_count(void)
{
    return g_tls_dtor;
}
