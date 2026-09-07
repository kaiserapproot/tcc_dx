// N6-05 H: TLS phase precedes atexit callbacks regardless of registration order.
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

struct Tls {
    Tls() {}
    ~Tls() {
        if (g_step != 0)
            n6_fail(81);
        log_step("TLS_DTOR");
        g_step = 1;
    }
};
thread_local Tls tls;

static void atexit_cb(void)
{
    if (g_step != 1)
        n6_fail(82);
    log_step("ATEXIT_CALLBACK");
    g_step = 2;
}

static void verify_order(void)
{
    if (g_step != 2)
        n6_fail(83);
    printf("ORDER_TLS_BEFORE_ATEXIT=PASS\n");
    fflush(stdout);
}

static void run_atexit_then_tls()
{
    g_step = 0;
    printf("CASE=ATEXIT_THEN_TLS\n");
    fflush(stdout);
    if (atexit(atexit_cb) != 0)
        return;
    (void)&tls;
}

static void run_tls_then_atexit()
{
    g_step = 0;
    printf("CASE=TLS_THEN_ATEXIT\n");
    fflush(stdout);
    (void)&tls;
    if (atexit(atexit_cb) != 0)
        return;
}

int main(int argc, char **argv)
{
    if (atexit(verify_order) != 0)
        return 1;
    if (argc > 1 && (argv[1][0] == '2' || argv[1][0] == 's'))
        run_tls_then_atexit();
    else
        run_atexit_then_tls();
    return 0;
}
