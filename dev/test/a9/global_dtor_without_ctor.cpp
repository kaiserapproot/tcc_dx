/* Global (and file-static) objects of a class that has a destructor but no
   user constructor still have the destructor run at exit, in reverse order
   of construction (C++98 3.6.3, 12.4/10).  Also when the destructor is
   only implicit (a member or a base has one).

   The destructors run after main returns, so the first object defined
   (Checker, destroyed last) checks the log and ends the process with
   _exit.  main returns 3: if Checker's destructor never runs, the test
   fails with 3.  Expected log = g++ -std=c++98 result. */
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
    ~Checker() { _exit(strcmp(g_log, "AAAbMBA") == 0 ? 0 : 1); }
};

struct DtorOnly { int v; ~DtorOnly() { note("A"); } };
struct WithCtor { int v; WithCtor() : v(1) {} ~WithCtor() { note("B"); } };
struct Member { ~Member() { note("M"); } };
struct Holder { Member m; int k; };            /* implicit dtor via a member */
struct Base { ~Base() { note("b"); } };
struct Derived : Base { };                     /* implicit dtor via a base */

Checker chk;        /* constructed first, destroyed last */
DtorOnly a;
WithCtor w;
Holder h;
static Derived d2;
static DtorOnly a2;
static DtorOnly arr2[2];                       /* array: "AA" */

int main()
{
    (void)chk; (void)a; (void)w; (void)h; (void)d2; (void)a2; (void)arr2;
    return 3;
}
