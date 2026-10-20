/* Loads the .so twice; each round registers one atexit() handler inside
   the library and dlclose()s it.  Expected stderr, per round:
   CLOSING / SO_ATEXIT / CLOSED (the handler runs at dlclose). */
#include <stdio.h>
#include <dlfcn.h>
int main(int argc, char **argv)
{
    int round;
    for (round = 0; round < 2; round++) {
        void *h = dlopen(argv[1], RTLD_NOW);
        void (*init)(void);
        if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
        init = (void (*)(void))dlsym(h, "so_initialize");
        if (!init) return 2;
        init();
        fputs("CLOSING\n", stderr);
        if (dlclose(h) != 0) return 3;
        fputs("CLOSED\n", stderr);
    }
    return 0;
}
