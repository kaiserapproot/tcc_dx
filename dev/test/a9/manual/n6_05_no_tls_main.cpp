// N6-05: main with no TLS still reaches FINALIZED (state=3) before process exit.
#include <stdio.h>
#include <stdlib.h>

extern "C" int __tcc_cpp_tls_n6_main_state(void);

static void check_finalized()
{
    int st;
    st = __tcc_cpp_tls_n6_main_state();
    printf("MAIN_STATE_AT_ATEXIT=%d\n", st);
    fflush(stdout);
    if (st != 3)
        exit(92);
}

int main()
{
    if (atexit(check_finalized) != 0)
        return 1;
    return 0;
}
