#include <stdio.h>
#include <dlfcn.h>
int main(int argc, char **argv)
{
    void *h = dlopen(argv[1], RTLD_NOW);
    void (*init)(void);
    if (!h) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
    init = (void (*)(void))dlsym(h, "so_initialize");
    if (!init) return 2;
    init();
    fputs("CLOSING\n", stderr);
    if (dlclose(h) != 0) return 3;
    fputs("CLOSED\n", stderr);
    return 0;
}
