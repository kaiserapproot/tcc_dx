/* Global objects that need destruction but were never registered, so
   their destructors never ran (C++98 3.6.3, 12.4/10):
   - objects with an initializer list (`A g = { 1 };`, const, arrays, and
     the out-of-class definition of a static data member `A S::m = { 9 };`)
     are initialized statically, but still destroyed at exit;
   - the out-of-class definition of a static data member without an
     initializer `W S::m;` (not an array) was neither constructed nor
     destroyed: the startup path wanted `;` or `(` right after the name.
   All are destroyed in the reverse order of their definitions.
   Checker (defined first, destroyed last) checks the log and ends the
   process with _exit; main returns 3 so a missing Checker fails.
   Expected log = g++ -std=c++98 result. */
#include <string.h>
#include <stdlib.h>          /* _exit on Windows */
#ifndef _WIN32
#include <unistd.h>          /* _exit on POSIX */
#endif

static char g_log[64];

static void note(const char *s)
{
    strcat(g_log, s);
}

struct Checker {
    ~Checker() { _exit(strcmp(g_log, "www|WW76532190W") == 0 ? 0 : 1); }
};
struct W { int v; W() : v(1) { note("w"); } ~W() { note("W"); } };
struct A { int v; ~A() { char b[2]; b[0] = (char)('0' + v); b[1] = 0; note(b); } };
struct S { static W sw; static A sa; static A si; static W swa[2]; };

Checker chk;
W S::sw;                        /* "w" ... "W" */
A S::sa;                        /* v == 0: "0" */
A S::si = { 9 };                /* "9" */
A ag = { 1 };                   /* "1" */
A aga[2] = { { 2 }, { 3 } };    /* "3", "2" */
const A ca = { 5 };             /* "5" */
static A sa2[1][2] = { { { 6 }, { 7 } } };  /* "7", "6" */
W S::swa[2];                    /* "ww" ... "WW" */

int main()
{
    if (S::sw.v != 1 || S::swa[1].v != 1 || S::si.v != 9)
        return 4;
    if (ag.v != 1 || aga[1].v != 3 || ca.v != 5 || sa2[0][1].v != 7)
        return 5;
    note("|");
    return 3;
}
