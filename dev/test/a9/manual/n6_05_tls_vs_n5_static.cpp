// N6-05 H: TLS destruction always precedes N5 local-static destruction.
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static int g_step;

static void log_step(const char *tag)
{
    printf("STEP %s\n", tag);
    fflush(stdout);
}

static void n6_fail(unsigned code)
{
    ExitProcess(code);
}

struct N5 {
    N5() {}
    ~N5() {
        if (g_step != 1)
            n6_fail(82);
        log_step("N5_STATIC_DTOR");
        g_step = 2;
    }
};
struct Tls {
    Tls() {}
    ~Tls() {
        if (g_step != 0)
            n6_fail(81);
        log_step("TLS_DTOR");
        g_step = 1;
    }
};

static N5 &n5_obj()
{
    static N5 s;
    return s;
}

static void verify_order(void)
{
    if (g_step != 2)
        n6_fail(83);
    printf("ORDER_TLS_BEFORE_N5=PASS\n");
    fflush(stdout);
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
    if (atexit(verify_order) != 0)
        return 1;
    if (argc > 1 && (argv[1][0] == '2' || argv[1][0] == 's'))
        run_tls_then_n5();
    else
        run_n5_then_tls();
    return 0;
}
