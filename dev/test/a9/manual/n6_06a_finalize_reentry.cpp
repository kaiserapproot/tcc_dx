// N6-06A: TLS dtor calling exit() during run finalize is fail-closed (-run).
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static void n6_06a_suppress_wer(void)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
}

struct A {
    A() {}
    ~A() { exit(9); }
};
thread_local A a;

int main()
{
    n6_06a_suppress_wer();
    (void)&a;
    return 0;
}
