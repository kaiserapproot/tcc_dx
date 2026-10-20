/* exit order must match glibc: atexit handlers (C++ global dtors are
   registered there, in reverse order) and then .fini_array.
   Output goes to stderr; run_tests.sh compares it.  argv[1] = "exit"
   leaves through exit() instead of returning from main. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct C { int k; C() { k = 1; } ~C() { fputs("GLOBAL_DTOR\n", stderr); } };
C c;
__attribute__((destructor)) static void fini(void) { fputs("FINI_ARRAY\n", stderr); }
static void ae(void) { fputs("ATEXIT\n", stderr); }
int main(int argc, char **argv)
{
    atexit(ae);
    fputs("MAIN\n", stderr);
    if (argc > 1 && strcmp(argv[1], "exit") == 0)
        exit(0);
    return 0;
}
