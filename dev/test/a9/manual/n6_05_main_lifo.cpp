// N6-05: main-thread TLS dtors drain LIFO by actual construction order.
#include <stdio.h>

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

int main()
{
    (void)&c;
    (void)&a;
    (void)&b;
    return 0;
}
