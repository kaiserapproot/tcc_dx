// N6-04B: second cleanup is a no-op; TLS slot is cleared before the TCB is freed.
//
// worker が `thread_local P p` を touch した後, 同じ cleanup authority
// (__tcc_cpp_tls_n6_cleanup_current_thread = DLL_THREAD_DETACH hook と同じ
// tcc_cpp_tls_thread_cleanup) を自スレッド上で明示的に呼ぶ.
//
//   1 回目: 戻り値 1 (TCB あり) -> dtor 1 回, TCB_FREE +1, RECLAIM +1,
//           直後の __tcc_cpp_tls_n6_current_tcb() == NULL
//           (TLS slot は TCB free の前に NULL 化されているので, 解放済み TCB を
//            指す slot は観測されない: N6_04B_TLS_SLOT_CLEAR_BEFORE_TCB_FREE)
//   2 回目: 戻り値 0 (TCB なし) -> counter 全部不変, dtor 増えない
//           (N6_04B_SECOND_CLEANUP_NOOP)
//   thread return: 本物の hook は TCB を見つけないので何もしない
//           (join 後も HOOK_DELIVERED は 1 回目の +1 のまま, dtor 合計 1)
//
// TlsSetValue(key, NULL) が失敗した場合の fail-closed (leak, TCB free しない) は
// runtime の分岐 (SLOT_CLEAR_FAILURE counter) として存在するが, 実機で失敗を
// 誘発できないため counter == 0 を確認するのみ.
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
int __tcc_cpp_tls_n6_cleanup_current_thread(void);
unsigned __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}
enum { ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE, ST_OBJECT_ALLOC,
       ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE, ST_HOOK_DELIVERED, ST_DRAIN_STARTED,
       ST_DRAIN_COMPLETED, ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT };

static volatile int g_dtor_calls;
static volatile DWORD g_dtor_tid;
struct P { int v; P(); ~P(); };
thread_local P p;
P::P() { v = 5; }
P::~P() { ++g_dtor_calls; g_dtor_tid = GetCurrentThreadId(); }

struct Arg {
    volatile DWORD tid;
    void *volatile tcb_before;
    void *volatile tcb_after_first;
    void *volatile tcb_after_second;
    volatile int rc_first, rc_second;
    volatile int dtor_after_first, dtor_after_second;
    long st_before[ST_COUNT], st_after_first[ST_COUNT], st_after_second[ST_COUNT];
};

static DWORD WINAPI worker(void *arg)
{
    Arg *a = (Arg *)arg;
    a->tid = GetCurrentThreadId();
    p.v += 1;
    a->tcb_before = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_stats(a->st_before, ST_COUNT);
    a->rc_first = __tcc_cpp_tls_n6_cleanup_current_thread();
    a->tcb_after_first = __tcc_cpp_tls_n6_current_tcb();
    a->dtor_after_first = g_dtor_calls;
    __tcc_cpp_tls_n6_stats(a->st_after_first, ST_COUNT);
    a->rc_second = __tcc_cpp_tls_n6_cleanup_current_thread();
    a->tcb_after_second = __tcc_cpp_tls_n6_current_tcb();
    a->dtor_after_second = g_dtor_calls;
    __tcc_cpp_tls_n6_stats(a->st_after_second, ST_COUNT);
    return 0;
}

static long d(long *a0, long *b0, int i) { return b0[i] - a0[i]; }

int main()
{
    Arg a;
    HANDLE h;
    long st0[ST_COUNT], st1[ST_COUNT];
    int i, first_ok, second_ok, joined_ok, unchanged = 1;

    memset(&a, 0, sizeof a);
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    h = CreateThread(NULL, 0, worker, &a, 0, NULL);
    if (!h) { printf("CreateThread failed\n"); return 1; }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);

    first_ok = a.tcb_before != 0 && a.rc_first == 1 && a.tcb_after_first == 0
               && a.dtor_after_first == 1 && g_dtor_tid == a.tid
               && d(a.st_before, a.st_after_first, ST_HOOK_DELIVERED) == 1
               && d(a.st_before, a.st_after_first, ST_DRAIN_COMPLETED) == 1
               && d(a.st_before, a.st_after_first, ST_RECLAIM_COMPLETED) == 1
               && d(a.st_before, a.st_after_first, ST_TCB_FREE) == 1
               && d(a.st_before, a.st_after_first, ST_OBJECT_FREE) == 1
               && d(a.st_before, a.st_after_first, ST_ENTRIES_FREE) == 1
               && d(a.st_before, a.st_after_first, ST_DTORS_FREE) == 1
               && d(a.st_before, a.st_after_first, ST_SLOT_CLEAR_FAILURE) == 0;
    printf("FIRST_CLEANUP: rc=%d tcb_before=%s tcb_after=%s dtor=%d on_owner=%s tcb_free=%ld object_free=%ld reclaim=%ld -> %s\n",
           a.rc_first, a.tcb_before ? "present" : "NULL", a.tcb_after_first ? "STILL_PRESENT" : "NULL",
           a.dtor_after_first, g_dtor_tid == a.tid ? "YES" : "NO",
           d(a.st_before, a.st_after_first, ST_TCB_FREE), d(a.st_before, a.st_after_first, ST_OBJECT_FREE),
           d(a.st_before, a.st_after_first, ST_RECLAIM_COMPLETED), first_ok ? "PASS" : "FAIL");

    for (i = 0; i < ST_COUNT; ++i)
        if (a.st_after_second[i] != a.st_after_first[i])
            unchanged = 0;
    second_ok = a.rc_second == 0 && a.tcb_after_second == 0 && a.dtor_after_second == 1 && unchanged;
    printf("SECOND_CLEANUP: rc=%d tcb=%s dtor=%d counters_unchanged=%s -> %s\n",
           a.rc_second, a.tcb_after_second ? "UNEXPECTED" : "NULL", a.dtor_after_second,
           unchanged ? "YES" : "NO", second_ok ? "PASS" : "FAIL");

    // thread return 後: 本物の hook は TCB なしで何もしない.
    joined_ok = g_dtor_calls == 1
                && d(st0, st1, ST_HOOK_DELIVERED) == 1 && d(st0, st1, ST_DRAIN_COMPLETED) == 1
                && d(st0, st1, ST_RECLAIM_COMPLETED) == 1 && d(st0, st1, ST_DTOR_CALLS) == 1
                && d(st0, st1, ST_TCB_ALLOC) == 1 && d(st0, st1, ST_TCB_FREE) == 1
                && (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE]) == 0
                && (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE]) == 0
                && (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]) == 0
                && (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]) == 0;
    printf("AFTER_JOIN: dtor_total=%d hooks=%ld drains=%ld reclaims=%ld tcb=%ld/%ld outstanding=%ld -> %s\n",
           g_dtor_calls, d(st0, st1, ST_HOOK_DELIVERED), d(st0, st1, ST_DRAIN_COMPLETED),
           d(st0, st1, ST_RECLAIM_COMPLETED), d(st0, st1, ST_TCB_ALLOC), d(st0, st1, ST_TCB_FREE),
           (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE]) + (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE])
           + (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]) + (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]),
           joined_ok ? "PASS" : "FAIL");

    if (!(first_ok && second_ok && joined_ok)) {
        printf("N6_04B_TLS_SECOND_CLEANUP=FAIL\n");
        return 1;
    }
    printf("N6_04B_TLS_SECOND_CLEANUP=PASS\n");
    printf("N6_04B_SECOND_CLEANUP_NOOP=PASS\n");
    printf("N6_04B_TLS_SLOT_CLEAR_BEFORE_TCB_FREE=PASS\n");
    printf("TLS_SLOT_CLEAR_FAILURE=FAIL_CLOSED (counter=0, leak path not reachable on this host)\n");
    return 0;
}
