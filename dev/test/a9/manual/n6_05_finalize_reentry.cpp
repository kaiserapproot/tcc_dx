// N6-05 I: TLS dtor calling exit() during main finalize is fail-closed.
#include <stdio.h>
#include <stdlib.h>

struct A {
    A() {}
    ~A() { exit(9); }
};
thread_local A a;

int main()
{
    (void)&a;
    return 0;
}
