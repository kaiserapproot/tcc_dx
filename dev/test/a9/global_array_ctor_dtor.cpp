/* Global arrays of a class with constructor and destructor: every element
   is constructed at its own address in order, and destroyed in reverse
   order at exit (C++98 12.6, 12.4/10).  The startup loop used to scale the
   element offset by the whole array's size, so elements past the first
   were constructed outside the array, and no element was destroyed.
   Checker (defined first, destroyed last) checks the log and ends the
   process with _exit; main returns 3 so a missing Checker fails.
   Expected log = g++ -std=c++98 result. */
#include <string.h>
#include <stdlib.h>          /* _exit on Windows */
#ifndef _WIN32
#include <unistd.h>          /* _exit on POSIX */
#endif

static char g_log[64];
static int g_next;

struct Checker {
    ~Checker() { _exit(strcmp(g_log, "cba210") == 0 ? 0 : 1); }
};
struct W {
    int id;
    W() : id(g_next++) {}
    ~W() { char b[2]; b[0] = (char)('0' + id); b[1] = 0; strcat(g_log, b); }
};
struct V {
    int id;
    V() : id(g_next++) {}
    ~V() { char b[2]; b[0] = (char)('a' + id - 3); b[1] = 0; strcat(g_log, b); }
};

Checker chk;
W arr[3];           /* ids 0, 1, 2 */
V two[1][2];        /* ids 3, 4: destroyed after last, as "b", "a" */
V last;             /* id 5: destroyed before everything else, "c" */

int main()
{
    if (arr[0].id != 0 || arr[1].id != 1 || arr[2].id != 2) return 4;
    if (two[0][0].id != 3 || two[0][1].id != 4 || last.id != 5) return 5;
    return 3;
}
