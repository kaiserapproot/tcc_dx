// N6-06B-00: manual get_symbol("main") semantics (not _runmain).
#include <stdio.h>

extern "C" {
unsigned __cdecl __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}

enum { ST_DTOR_CALLS = 14, ST_COUNT = 17 };

static volatile int g_tls_dtor;

struct MainTls {
    MainTls() {}
    ~MainTls() {
        ++g_tls_dtor;
        printf("MANUAL_MAIN_TLS_DTOR\n");
        fflush(stdout);
    }
};
thread_local MainTls main_tls;

static void touch_main_tls(void)
{
    (void)&main_tls;
}

extern "C" int main(void)
{
    touch_main_tls();
    printf("MANUAL_MAIN_BODY\n");
    fflush(stdout);
    return 17;
}

extern "C" int probe_main_tls_dtor_count(void)
{
    return g_tls_dtor;
}

extern "C" long probe_main_dtor_calls(void)
{
    long st[ST_COUNT];
    __tcc_cpp_tls_n6_stats(st, ST_COUNT);
    return st[ST_DTOR_CALLS];
}
