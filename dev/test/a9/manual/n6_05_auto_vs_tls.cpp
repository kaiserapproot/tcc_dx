// N6-05 G1: automatic storage dtors run during main return before TLS finalize.
#include <stdio.h>

static int g_step;

static void log_step(const char *tag)
{
    printf("STEP %s\n", tag);
    fflush(stdout);
}

struct Auto {
    Auto() {}
    ~Auto() { log_step("AUTO"); }
};
struct Tls {
    Tls() {}
    ~Tls() { log_step("TLS"); }
};
thread_local Tls tls;

int main()
{
    (void)&tls;
    Auto a;
    (void)a;
    return 17;
}
