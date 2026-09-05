// N6-02 REVIEW FIX-4: indirect recursive TLS initialization
// (TLS A ctor -> TLS B, TLS B ctor -> TLS A).
//
// A の構築中 (INITIALIZING) に B の構築が始まり, B の ctor が A に戻ってくる.
// runtime は A の entry を state==INITIALIZING で見つけ abort() する.
// 期待: A ctor 1 回, B ctor 1 回, UNEXPECTED_RETURN なし, exit 42.
#include <windows.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static int g_a_ctor_entered;
static int g_b_ctor_entered;

struct A {
    int v;
    A();
};

struct B {
    int v;
    B();
};

thread_local A object_a;
thread_local B object_b;

A::A()
{
    ++g_a_ctor_entered;
    v = object_b.v + 1;
}

B::B()
{
    ++g_b_ctor_entered;
    v = object_a.v + 1;
}

static void on_abort(int sig)
{
    printf("SIGABRT_RECEIVED=%d\n", sig);
    printf("A_CTOR_ENTERED=%d\n", g_a_ctor_entered);
    printf("B_CTOR_ENTERED=%d\n", g_b_ctor_entered);
    if (g_a_ctor_entered == 1 && g_b_ctor_entered == 1)
        printf("N6_02_RECURSIVE_INIT_INDIRECT=ABORT_FAIL_CLOSED\n");
    else
        printf("N6_02_RECURSIVE_INIT_INDIRECT=DOUBLE_CONSTRUCTION\n");
    fflush(stdout);
    ExitProcess(42);
}

int main()
{
    int v;

    signal(SIGABRT, on_abort);
    printf("RECURSION_ATTEMPT=INDIRECT\n");
    fflush(stdout);
    v = object_a.v;
    printf("UNEXPECTED_RETURN value=%d a=%d b=%d\n", v, g_a_ctor_entered, g_b_ctor_entered);
    return 1;
}
