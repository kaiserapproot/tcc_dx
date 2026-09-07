// N6-05 G1: automatic storage dtors run during main return before TLS finalize.
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

struct Auto {
    Auto() {}
    ~Auto() {
        if (g_step != 0)
            n6_fail(81);
        log_step("AUTO");
        g_step = 1;
    }
};
struct Tls {
    Tls() {}
    ~Tls() {
        if (g_step != 1)
            n6_fail(82);
        log_step("TLS");
        g_step = 2;
    }
};
thread_local Tls tls;

static void verify_order(void)
{
    if (g_step != 2)
        n6_fail(83);
    printf("ORDER_AUTO_BEFORE_TLS=PASS\n");
    fflush(stdout);
}

int main()
{
    if (atexit(verify_order) != 0)
        return 1;
    (void)&tls;
    Auto a;
    (void)a;
    return 17;
}
