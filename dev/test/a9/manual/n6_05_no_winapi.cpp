// Minimal n6_05 without windows.h (TLS order test).
#include <stdio.h>
#include <stdlib.h>

static int g_step;

static void log_step(const char *tag)
{
    printf("STEP %s\n", tag);
    fflush(stdout);
}

static void n6_fail(unsigned code)
{
    exit(code);
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

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (atexit(verify_order) != 0)
        return 1;
    g_step = 0;
    (void)&tls;
    if (atexit(atexit_cb) != 0)
        return 1;
    return 0;
}
