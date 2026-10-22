/* Global objects defined in an #include'd header were neither constructed
   nor destroyed: the startup registration accepted the primary source file
   only.  It now accepts real #include files too, like global copy-init.
   Checker (defined before the #include, destroyed last) checks that all
   five header objects were destroyed and ends the process with _exit;
   main returns 3 so a missing Checker fails. */
#include <stdlib.h>          /* _exit on Windows */
#ifndef _WIN32
#include <unistd.h>          /* _exit on POSIX */
#endif

int g_destroyed = 0;

struct Checker {
    ~Checker() { _exit(g_destroyed == 5 ? 0 : 1); }
};
Checker chk;

#include "header_global_ctor_dtor.h"

int main()
{
    if (h_def.v != 7 || h_args[1].v != 7 || HS::sw.v != 7)
        return 4;
    if (h_init.value != 123)
        return 5;
    return 3;
}
