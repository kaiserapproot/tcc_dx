#include <stdio.h>
#include <dlfcn.h>
int main(int argc, char **argv)
{
    int round;
    for (round = 0; round < 2; round++) {
        void *h = dlopen(argv[1], RTLD_NOW);
        int (*f)(int);
        if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
        f = (int (*)(int))dlsym(h, "so_cpp_get");
        if (!f || f(12) != 42) return 2;   /* global ctor ran in the .so */
        if (dlclose(h) != 0) return 3;
    }
    return 0;
}
