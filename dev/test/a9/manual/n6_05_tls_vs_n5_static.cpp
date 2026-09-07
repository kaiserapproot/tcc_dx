// N6-05 H: TLS destruction always precedes N5 local-static destruction.
// Pass argv[1]=1 (N5 then TLS) or 2 (TLS then N5); both must log TLS before N5.
#include <stdio.h>

static int g_step;

static void log_step(const char *tag)
{
    printf("STEP %s\n", tag);
    fflush(stdout);
}

struct N5 {
    N5() {}
    ~N5() { log_step("N5_STATIC_DTOR"); }
};
struct Tls {
    Tls() {}
    ~Tls() { log_step("TLS_DTOR"); }
};

static N5 &n5_obj()
{
    static N5 s;
    return s;
}

static void run_n5_then_tls()
{
    g_step = 0;
    printf("CASE=N5_THEN_TLS\n");
    fflush(stdout);
    (void)&n5_obj();
    thread_local Tls tls;
    (void)&tls;
}

static void run_tls_then_n5()
{
    g_step = 0;
    printf("CASE=TLS_THEN_N5\n");
    fflush(stdout);
    thread_local Tls tls;
    (void)&tls;
    (void)&n5_obj();
}

int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == '2')
        run_tls_then_n5();
    else
        run_n5_then_tls();
    return 0;
}
