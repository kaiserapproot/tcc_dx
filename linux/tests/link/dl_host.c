/* dlopen / dlsym / dlclose, loaded twice (re-initialisation on reload) */
#include <stdio.h>
#include <dlfcn.h>
int main(int argc, char **argv)
{
    int round;
    for (round = 0; round < 2; round++) {
        void *h = dlopen(argv[1], RTLD_NOW);
        int (*f)(int);
        int *v;
        if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
        f = (int (*)(int))dlsym(h, "so_get");
        v = (int *)dlsym(h, "so_value");
        if (!f || !v) return 2;
        if (f(1) != 11) return 3;          /* ctor ran on this load */
        *v = 20;
        if (f(1) != 21) return 4;
        if (dlclose(h) != 0) return 5;
    }
    fputs("HOST_END\n", stderr);
    return 0;
}
