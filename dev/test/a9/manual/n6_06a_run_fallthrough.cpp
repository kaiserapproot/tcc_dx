// N6-06A: implicit return 0 still runs TLS finalize on tcc_run return path.
#include <stdio.h>

struct Tls {
    Tls() {}
    ~Tls() { printf("TLS_DTOR\n"); fflush(stdout); }
};
thread_local Tls tls;

int main()
{
    (void)&tls;
}
