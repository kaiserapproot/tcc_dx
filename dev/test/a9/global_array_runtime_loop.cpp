/* The startup / exit thunks walk a global class array with a loop at run
   time instead of one call per element (one call per element made a
   100000-element array megabytes of code).  Every element is constructed
   in order and destroyed in reverse order, for a multidimensional array,
   for an element larger than 8 bytes, and for a large array.
   Checker (defined first, destroyed last) checks the log and ends the
   process with _exit; main returns 3 so a missing Checker fails.
   Expected log = g++ -std=c++98 result. */
#include <string.h>
#include <stdlib.h>          /* _exit on Windows */
#ifndef _WIN32
#include <unistd.h>          /* _exit on POSIX */
#endif

#define NBIG 20000

static char g_log[64];
static int g_next;
static int g_big_next;
static int g_big_bad;
static int g_big_left;

static void note(int c)
{
    char b[2];

    b[0] = (char)c;
    b[1] = 0;
    strcat(g_log, b);
}

struct Checker {
    ~Checker() {
        _exit(strcmp(g_log, "|DCBAfedcba") == 0 && !g_big_bad
              && g_big_left == 0 ? 0 : 1);
    }
};
struct E { int id; E() : id(g_next++) {} ~E() { note('a' + id); } };
struct Big { char pad[40]; int id; Big() : id(g_next++ - 6) {} ~Big() { note('A' + id); } };
struct Many {
    int id;
    Many() : id(g_big_next++) {}
    /* destroyed last to first: ids count down to 0 */
    ~Many() { if (id != --g_big_left) g_big_bad = 1; }
};

Checker chk;
Many many[NBIG];     /* destroyed after bg and e3 */
E e3[2][3];          /* ids 0..5: "fedcba" */
Big bg[4];           /* ids 0..3: "DCBA" */

int main()
{
    int i;

    for (i = 0; i < NBIG; i++)
        if (many[i].id != i)
            return 4;
    if (e3[1][2].id != 5 || bg[3].id != 3)
        return 5;
    g_big_left = NBIG;
    strcat(g_log, "|");
    return 3;
}
