// N6-02 REVIEW FIX-4: direct recursive TLS initialization (TLS A ctor -> TLS A).
//
// SPEC §3.2: INITIALIZING 中に同じ thread から同じ object を再初期化しようとする
// recursive initialization は, 未初期化値返却 / 二重構築 / global 経路への
// fallback を禁止し, 診断または安全な失敗で fail-closed する.
// runtime (__tcc_cpp_tls_addr) は state==INITIALIZING の entry を見つけると
// abort() する. abort は SIGABRT を raise するので, handler で
//   - ctor が 1 回しか入っていないこと (二重構築なし)
//   - 「値が返った」経路 (UNEXPECTED_RETURN) に到達していないこと
// を出力し, exit 42 で終了する (msvcrt の既定 abort 終了処理へは進ませない).
#include <windows.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static int g_a_ctor_entered;

struct A {
    int v;
    A();
};

thread_local A object_a;

// 自分自身 (構築中) を参照する -> runtime が INITIALIZING を検出して abort.
A::A()
{
    ++g_a_ctor_entered;
    v = object_a.v + 1;
}

static void on_abort(int sig)
{
    printf("SIGABRT_RECEIVED=%d\n", sig);
    printf("A_CTOR_ENTERED=%d\n", g_a_ctor_entered);
    if (g_a_ctor_entered == 1)
        printf("N6_02_RECURSIVE_INIT_DIRECT=ABORT_FAIL_CLOSED\n");
    else
        printf("N6_02_RECURSIVE_INIT_DIRECT=DOUBLE_CONSTRUCTION\n");
    fflush(stdout);
    ExitProcess(42);
}

int main()
{
    int v;

    signal(SIGABRT, on_abort);
    printf("RECURSION_ATTEMPT=DIRECT\n");
    fflush(stdout);
    v = object_a.v;
    // ここへ来たら fail-closed されていない (未初期化値返却 or 二重構築).
    printf("UNEXPECTED_RETURN value=%d ctor_entered=%d\n", v, g_a_ctor_entered);
    return 1;
}
