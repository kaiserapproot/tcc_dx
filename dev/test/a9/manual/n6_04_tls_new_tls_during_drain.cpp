// N6-04 fail-closed: destructor drain 中の「未構築 TLS の初回初期化」は abort する
// (NEW_TLS_INITIALIZATION_DURING_DRAIN=FAIL_CLOSED,
//  NEW_DTOR_REGISTRATION_DURING_DRAIN=FAIL_CLOSED).
//
// worker は hot だけを touch する.  A::~A() (thread exit の drain 中) が, その thread で
// 一度も触っていない cold を初期化しようとする -> runtime (__tcc_cpp_tls_addr) は
// tcb->cleanup_state != 0 を見て abort() する.  cold の ctor は走らず, cold の dtor が
// registry に追加されることもない (registry が drain 中に増殖しない).
//
// abort は SIGABRT を raise するので handler で
//   - COLD_CTOR_ENTERED == 0   (drain 中に新規構築されていない)
//   - HOT_DTOR_ENTERED  == 1   (drain 自体は開始している)
// を出力し exit code 42 で終了する (n6_02_tls_recursive_init と同じ判定方式).
// handler は exit 中の worker thread = Windows loader notification context 上で
// 走るため, ExitProcess (DLL_PROCESS_DETACH 通知 -> loader lock 再取得) ではなく
// TerminateProcess で終了する.  実測: ExitProcess(42) はこの context から
// 0xC0000409 になった (2026-09-06).  spec 8.3 のとおり worker からの exit は
// N6 の対象外であり, ここでは test 終了手段としてだけ使う.
#include <windows.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

static volatile int g_hot_ctor, g_hot_dtor;
static volatile int g_cold_ctor, g_cold_dtor;
static volatile int g_cold_value_seen;

struct Hot { int v; Hot(); ~Hot(); };
struct Cold { int v; Cold(); ~Cold(); };

thread_local Hot hot;
thread_local Cold cold;

Hot::Hot() { v = 1; ++g_hot_ctor; }
Cold::Cold() { v = 2; ++g_cold_ctor; }
Cold::~Cold() { ++g_cold_dtor; }

Hot::~Hot()
{
    ++g_hot_dtor;
    // drain 中の新規 TLS 初期化 -> fail-closed abort. ここから戻ってはならない.
    g_cold_value_seen = cold.v;
    printf("UNEXPECTED_RETURN cold.v=%d cold_ctor=%d\n", g_cold_value_seen, g_cold_ctor);
    fflush(stdout);
}

static void on_abort(int sig)
{
    printf("SIGABRT_RECEIVED=%d\n", sig);
    printf("HOT_DTOR_ENTERED=%d\n", g_hot_dtor);
    printf("COLD_CTOR_ENTERED=%d\n", g_cold_ctor);
    printf("COLD_DTOR_ENTERED=%d\n", g_cold_dtor);
    if (g_hot_dtor == 1 && g_cold_ctor == 0 && g_cold_dtor == 0)
        printf("N6_04_NEW_TLS_DURING_DRAIN=ABORT_FAIL_CLOSED\n");
    else
        printf("N6_04_NEW_TLS_DURING_DRAIN=WRONG_STATE\n");
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), 42);
}

static DWORD WINAPI worker(void *)
{
    hot.v += 1;
    return 0;
}

int main()
{
    HANDLE h;

    signal(SIGABRT, on_abort);
    printf("DRAIN_NEW_TLS_ATTEMPT=COLD_FROM_HOT_DTOR\n");
    fflush(stdout);
    h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (!h) { printf("CreateThread failed\n"); return 1; }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    // ここへ来たら fail-closed されていない.
    printf("UNEXPECTED_JOIN hot_dtor=%d cold_ctor=%d\n", g_hot_dtor, g_cold_ctor);
    return 1;
}
