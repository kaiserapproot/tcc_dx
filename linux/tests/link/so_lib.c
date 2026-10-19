/* shared library: exported function and variable, ELF ctor / dtor */
#include <stdio.h>
int so_value = 10;
static int so_inited;
__attribute__((constructor)) static void so_init(void) { so_inited++; }
__attribute__((destructor)) static void so_fini(void) { fputs("SO_FINI\n", stderr); }
int so_get(int x) { return so_inited ? x + so_value : -1; }
