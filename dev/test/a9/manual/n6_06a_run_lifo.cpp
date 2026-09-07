// N6-06A: main-thread TLS dtors drain LIFO on tcc_run return path.
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

static char g_order[8];
static int g_n;

static void log_tag(char tag)
{
    if (g_n < 8)
        g_order[g_n++] = tag;
    printf("DTOR_%c\n", tag);
    fflush(stdout);
}

struct A { A() {} ~A() { log_tag('A'); } };
struct B { B() {} ~B() { log_tag('B'); } };
struct C { C() {} ~C() { log_tag('C'); } };

thread_local A a;
thread_local C c;
thread_local B b;

static void verify_lifo(void)
{
    if (g_n != 3 || g_order[0] != 'B' || g_order[1] != 'A' || g_order[2] != 'C')
        ExitProcess(84);
    printf("LIFO_ORDER=PASS\n");
    fflush(stdout);
}

int main()
{
    if (atexit(verify_lifo) != 0)
        return 1;
    (void)&c;
    (void)&a;
    (void)&b;
    return 0;
}
