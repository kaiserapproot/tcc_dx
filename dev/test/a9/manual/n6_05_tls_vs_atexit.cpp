// N6-05 H: TLS phase precedes atexit callbacks regardless of registration order.
#include <stdio.h>
#include <stdlib.h>

static int g_step;

static void log_step(const char *tag)
{
    printf("STEP %s\n", tag);
    fflush(stdout);
}

struct Tls {
    Tls() {}
    ~Tls() { log_step("TLS_DTOR"); }
};
thread_local Tls tls;

static void atexit_cb()
{
    log_step("ATEXIT_CALLBACK");
}

static void run_atexit_then_tls()
{
    g_step = 0;
    printf("CASE=ATEXIT_THEN_TLS\n");
    fflush(stdout);
    if (atexit(atexit_cb) != 0)
        exit(1);
    (void)&tls;
}

static void run_tls_then_atexit()
{
    g_step = 0;
    printf("CASE=TLS_THEN_ATEXIT\n");
    fflush(stdout);
    (void)&tls;
    if (atexit(atexit_cb) != 0)
        exit(1);
}

int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == '2')
        run_tls_then_atexit();
    else
        run_atexit_then_tls();
    return 0;
}
