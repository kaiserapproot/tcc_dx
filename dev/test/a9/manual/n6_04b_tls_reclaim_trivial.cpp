// N6-04B Gate A: trivial thread_local (no ctor / no dtor) is still reclaimed.
//
//   thread_local int x;   worker が touch -> exit.
//
// dtor registry は空 (dtor_count = 0) だが TCB / entry array / x の storage は
// 存在するので, destructor phase が空でも memory reclaim phase は必ず走ること
// (「registry が空だから return」の禁止) を accounting counter で証明する.
//
// 測定 (counter は runtime 内部の test-only accounting, working set ではない):
//   N6_04B_TRIVIAL_RECLAIM          TCB_ALLOC=1 OBJECT_ALLOC=1 ENTRIES_ALLOC=1
//                                   DTORS_ALLOC=0 CTOR=none DTOR=none ->
//                                   join 後 TCB_FREE=1 OBJECT_FREE=1 ENTRIES_FREE=1
//                                   OUTSTANDING=0
//   N6_04B_UNACCESSED_TLS_NO_ALLOCATION
//                                   TLS に一切触らない worker では全 counter が不変
//                                   (TCB も storage も確保されない); 同じ TU にある
//                                   `thread_local Cold cold;` は誰も触らないので
//                                   OBJECT_ALLOC は x の 1 だけ
//   N6_04B_TRIVIAL_SEQUENTIAL       worker 3 本逐次 -> 各 1/1
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
unsigned __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}
enum { ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE, ST_OBJECT_ALLOC,
       ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE, ST_HOOK_DELIVERED, ST_DRAIN_STARTED,
       ST_DRAIN_COMPLETED, ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
       ST_POST_FINALIZE_OBJECT, ST_COUNT };

thread_local int x;

struct Cold {
    int v;
    Cold();
    ~Cold();
};
thread_local Cold cold;
static volatile int g_cold_ctor, g_cold_dtor;
Cold::Cold() { v = 1; ++g_cold_ctor; }
Cold::~Cold() { ++g_cold_dtor; }

struct Arg {
    int touch;
    volatile int seen;
    void *volatile tcb;
    long st_alive[ST_COUNT];
};

static DWORD WINAPI worker(void *p)
{
    Arg *a = (Arg *)p;
    if (a->touch) {
        x = 41;
        x += 1;
        a->seen = x;
        a->tcb = __tcc_cpp_tls_n6_current_tcb();
    } else {
        a->tcb = __tcc_cpp_tls_n6_current_tcb();
    }
    __tcc_cpp_tls_n6_stats(a->st_alive, ST_COUNT);
    return 0;
}

static int g_fail;

static long d(long *a, long *b, int i) { return b[i] - a[i]; }

static void run_trivial(const char *name)
{
    Arg a;
    HANDLE h;
    long st0[ST_COUNT], st1[ST_COUNT];
    int alive_ok, joined_ok, outstanding_ok;
    memset(&a, 0, sizeof a);
    a.touch = 1;
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    h = CreateThread(NULL, 0, worker, &a, 0, NULL);
    if (!h) { printf("%s: CreateThread failed\n", name); g_fail = 1; return; }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    // 生存中 (worker 末尾): 確保だけ, 解放なし, dtor registry なし.
    alive_ok = a.tcb != 0 && a.seen == 42
               && d(st0, a.st_alive, ST_TCB_ALLOC) == 1 && d(st0, a.st_alive, ST_TCB_FREE) == 0
               && d(st0, a.st_alive, ST_OBJECT_ALLOC) == 1 && d(st0, a.st_alive, ST_OBJECT_FREE) == 0
               && d(st0, a.st_alive, ST_ENTRIES_ALLOC) == 1 && d(st0, a.st_alive, ST_ENTRIES_FREE) == 0
               && d(st0, a.st_alive, ST_DTORS_ALLOC) == 0 && d(st0, a.st_alive, ST_DTOR_CALLS) == 0;
    // join 後: hook 1, drain 1 (空), reclaim 1, 全 category alloc == free.
    joined_ok = d(st0, st1, ST_HOOK_DELIVERED) == 1 && d(st0, st1, ST_DRAIN_COMPLETED) == 1
                && d(st0, st1, ST_RECLAIM_COMPLETED) == 1 && d(st0, st1, ST_DTOR_CALLS) == 0
                && d(st0, st1, ST_TCB_FREE) == 1 && d(st0, st1, ST_OBJECT_FREE) == 1
                && d(st0, st1, ST_ENTRIES_FREE) == 1 && d(st0, st1, ST_DTORS_ALLOC) == 0
                && d(st0, st1, ST_DTORS_FREE) == 0
                && d(st0, st1, ST_SLOT_CLEAR_FAILURE) == 0
                && d(st0, st1, ST_CROSS_THREAD_RECLAIM_SKIPPED) == 0;
    outstanding_ok = (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE]) == 0
                     && (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE]) == 0
                     && (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]) == 0
                     && (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]) == 0;
    printf("%s: CTOR=none DTOR=none TCB_ALLOC=%ld OBJECT_ALLOC=%ld ENTRIES_ALLOC=%ld DTORS_ALLOC=%ld "
           "THREAD_EXIT TCB_RECLAIM=%ld OBJECT_RECLAIM=%ld ENTRIES_RECLAIM=%ld dtor_calls=%ld "
           "OUTSTANDING=%ld -> %s\n",
           name, d(st0, st1, ST_TCB_ALLOC), d(st0, st1, ST_OBJECT_ALLOC),
           d(st0, st1, ST_ENTRIES_ALLOC), d(st0, st1, ST_DTORS_ALLOC),
           d(st0, st1, ST_TCB_FREE), d(st0, st1, ST_OBJECT_FREE), d(st0, st1, ST_ENTRIES_FREE),
           d(st0, st1, ST_DTOR_CALLS),
           (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE]) + (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE])
           + (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]) + (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]),
           (alive_ok && joined_ok && outstanding_ok) ? "PASS" : "FAIL");
    if (!(alive_ok && joined_ok && outstanding_ok))
        g_fail = 1;
}

int main()
{
    Arg none;
    HANDLE h;
    long st0[ST_COUNT], st1[ST_COUNT];
    int i, ok;

    // 1. trivial TLS: touch -> exit -> fully reclaimed.
    run_trivial("N6_04B_TRIVIAL_RECLAIM");

    // 2. TLS に触らない worker: 何も確保されず, 何も解放されない.
    memset(&none, 0, sizeof none);
    none.touch = 0;
    none.tcb = (void *)1;
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    h = CreateThread(NULL, 0, worker, &none, 0, NULL);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    ok = none.tcb == 0;
    for (i = 0; i < ST_COUNT; ++i)
        if (st1[i] != st0[i])
            ok = 0;
    printf("N6_04B_UNACCESSED_TLS_NO_ALLOCATION: tcb=%s counters_unchanged=%s -> %s\n",
           none.tcb == 0 ? "none" : "UNEXPECTED", ok ? "YES" : "NO", ok ? "PASS" : "FAIL");
    if (!ok)
        g_fail = 1;

    // 3. sequential recreate.
    for (i = 0; i < 3; ++i) {
        char nm[48];
        sprintf(nm, "N6_04B_TRIVIAL_SEQUENTIAL_%d", i);
        run_trivial(nm);
    }

    printf("COLD_CTOR=%d COLD_DTOR=%d (never touched)\n", g_cold_ctor, g_cold_dtor);
    if (g_cold_ctor != 0 || g_cold_dtor != 0)
        g_fail = 1;
    // main は TLS に触っていない.
    if (__tcc_cpp_tls_n6_current_tcb() != 0) {
        printf("MAIN_HAS_UNEXPECTED_TCB\n");
        g_fail = 1;
    }

    if (g_fail) {
        printf("N6_04B_TLS_RECLAIM_TRIVIAL=FAIL\n");
        return 1;
    }
    printf("N6_04B_TLS_RECLAIM_TRIVIAL=PASS\n");
    printf("N6_04B_TRIVIAL_RECLAIM=PASS\n");
    printf("N6_04B_UNACCESSED_TLS_NO_ALLOCATION=PASS\n");
    printf("EMPTY_DTOR_REGISTRY_SKIPS_RECLAIM=NO\n");
    return 0;
}
