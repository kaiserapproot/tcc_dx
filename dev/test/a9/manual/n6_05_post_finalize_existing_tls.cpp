// N6-05 G3: after main finalize, accessing destroyed main-thread TLS is fail-closed.
// TCC does not emit global static C++ dtors yet; atexit runs in the same post-TLS
// termination phase as static destructors for ordering gates (see tls_vs_atexit).
#include <stdio.h>
#include <stdlib.h>

struct X {
    X() {}
    ~X() {}
    int v;
};
thread_local X x;

static void touch_x()
{
    x.v = 1;
    (void)x.v;
}

static void gate_cb()
{
    touch_x();
    printf("GATE_DTOR\n");
    fflush(stdout);
}

int main()
{
    if (atexit(gate_cb) != 0)
        return 1;
    touch_x();
    return 0;
}
