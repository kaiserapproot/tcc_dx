// N6-05 G3: after main finalize, accessing destroyed main-thread TLS is fail-closed.
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static void n6_05_suppress_wer(void)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
}

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
    n6_05_suppress_wer();
    if (atexit(gate_cb) != 0)
        return 1;
    touch_x();
    return 0;
}
