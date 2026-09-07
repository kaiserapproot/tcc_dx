// N6-04B Gate B: non-trivial LIFO drain, then (and only then) storage reclaim.
//
// 1 worker が A, C, B の順に初回 touch (宣言順 A, B, C とは異なる) し, さらに
// trivial な `thread_local int x` にも触る.  期待:
//   construction : A, C, B
//   destruction  : B, C, A          (LIFO, 実構築順の逆)
//   --- drain complete ---
//   storage reclaim (A, B, C, x の 4 object + entry array + dtor array)
//   TCB reclaim
//
// N6_04B_NO_RECLAIM_BEFORE_DTOR_DRAIN
//   各 dtor は呼ばれた時点の accounting counter を記録する. drain 中に
//   OBJECT_FREE / ENTRIES_FREE / DTORS_FREE / TCB_FREE が 1 件でも増えていれば FAIL.
// N6_04B_EXISTING_ACCESS_DURING_DRAIN
//   B::~B は先に構築された a (まだ生存) と x を読む. a.v == 0xA, x == 7 のまま
//   (storage が drain 中に解放されていない実証).  C::~C は a を, A::~A は x を読む.
// N6_04B_MULTI_OBJECT_RECLAIM
//   join 後: OBJECT_ALLOC=4 OBJECT_FREE=4 ENTRIES 1/1 DTORS 1/1 TCB 1/1, OUTSTANDING=0.
// N6_04B_NONTRIVIAL_RECLAIM
//   上記全部 + dtor_calls=3 + DTOR_ORDER=B,C,A + 全 dtor が owner thread 上.
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

struct Record {
    volatile DWORD tid;
    char ctor_order[8];
    volatile int ctor_n;
    char dtor_order[8];
    volatile int dtor_n;
    DWORD dtor_tid[8];
    long st_at_dtor[3][ST_COUNT];   // counter snapshot inside each dtor
    volatile int a_seen_in_B;       // B::~B reads a.v (expected 0xA)
    volatile int x_seen_in_B;       // B::~B reads x   (expected 7)
    volatile int a_seen_in_C;       // C::~C reads a.v (expected 0xA)
    volatile int x_seen_in_A;       // A::~A reads x   (expected 7)
    long st_alive[ST_COUNT];
};
static Record g_rec;

thread_local int x;

struct A { int v; A(); ~A(); };
struct B { int v; B(); ~B(); };
struct C { int v; C(); ~C(); };

thread_local A a;
thread_local B b;
thread_local C c;

static void log_ctor(char tag)
{
    if (g_rec.ctor_n < 8)
        g_rec.ctor_order[g_rec.ctor_n] = tag;
    ++g_rec.ctor_n;
}
static void log_dtor(char tag)
{
    int n = g_rec.dtor_n;
    if (n < 3)
        __tcc_cpp_tls_n6_stats(g_rec.st_at_dtor[n], ST_COUNT);
    if (n < 8) {
        g_rec.dtor_order[n] = tag;
        g_rec.dtor_tid[n] = GetCurrentThreadId();
    }
    ++g_rec.dtor_n;
}

A::A() { v = 0xA; log_ctor('A'); }
A::~A() { g_rec.x_seen_in_A = x; log_dtor('A'); }
B::B() { v = 0xB; log_ctor('B'); }
B::~B() { g_rec.a_seen_in_B = a.v; g_rec.x_seen_in_B = x; log_dtor('B'); }
C::C() { v = 0xC; log_ctor('C'); }
C::~C() { g_rec.a_seen_in_C = a.v; log_dtor('C'); }

static DWORD WINAPI worker(void *)
{
    g_rec.tid = GetCurrentThreadId();
    x = 7;
    a.v += 0;
    c.v += 0;
    b.v += 0;
    // 再 touch: ctor / registry は増えない.
    a.v += 0; b.v += 0; c.v += 0;
    __tcc_cpp_tls_n6_stats(g_rec.st_alive, ST_COUNT);
    return 0;
}

static long d(long *a0, long *b0, int i) { return b0[i] - a0[i]; }

int main()
{
    HANDLE h;
    long st0[ST_COUNT], st1[ST_COUNT];
    int fail = 0, i;
    int order_ok, tid_ok = 1, no_reclaim_ok = 1, access_ok, alive_ok, reclaim_ok, outstanding_ok;

    memset(&g_rec, 0, sizeof g_rec);
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    h = CreateThread(NULL, 0, worker, 0, 0, NULL);
    if (!h) { printf("CreateThread failed\n"); return 1; }
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);

    g_rec.ctor_order[3] = 0;
    g_rec.dtor_order[3] = 0;
    order_ok = g_rec.ctor_n == 3 && g_rec.dtor_n == 3
               && strcmp(g_rec.ctor_order, "ACB") == 0 && strcmp(g_rec.dtor_order, "BCA") == 0;
    for (i = 0; i < 3; ++i)
        if (g_rec.dtor_tid[i] != g_rec.tid)
            tid_ok = 0;
    printf("CTOR_ORDER=%c,%c,%c DTOR_ORDER=%c,%c,%c dtor_tids_eq_owner=%s -> %s\n",
           g_rec.ctor_order[0], g_rec.ctor_order[1], g_rec.ctor_order[2],
           g_rec.dtor_order[0], g_rec.dtor_order[1], g_rec.dtor_order[2],
           tid_ok ? "YES" : "NO", (order_ok && tid_ok) ? "PASS" : "FAIL");

    // 生存中 (worker 末尾): 4 object, entry array, dtor array, TCB を確保, 解放 0.
    alive_ok = d(st0, g_rec.st_alive, ST_TCB_ALLOC) == 1 && d(st0, g_rec.st_alive, ST_OBJECT_ALLOC) == 4
               && d(st0, g_rec.st_alive, ST_ENTRIES_ALLOC) == 1 && d(st0, g_rec.st_alive, ST_DTORS_ALLOC) == 1
               && d(st0, g_rec.st_alive, ST_TCB_FREE) == 0 && d(st0, g_rec.st_alive, ST_OBJECT_FREE) == 0
               && d(st0, g_rec.st_alive, ST_ENTRIES_FREE) == 0 && d(st0, g_rec.st_alive, ST_DTORS_FREE) == 0
               && d(st0, g_rec.st_alive, ST_DTOR_CALLS) == 0;
    printf("ALIVE: tcb=%ld objects=%ld entries=%ld dtors=%ld frees=%ld -> %s\n",
           d(st0, g_rec.st_alive, ST_TCB_ALLOC), d(st0, g_rec.st_alive, ST_OBJECT_ALLOC),
           d(st0, g_rec.st_alive, ST_ENTRIES_ALLOC), d(st0, g_rec.st_alive, ST_DTORS_ALLOC),
           d(st0, g_rec.st_alive, ST_TCB_FREE) + d(st0, g_rec.st_alive, ST_OBJECT_FREE)
           + d(st0, g_rec.st_alive, ST_ENTRIES_FREE) + d(st0, g_rec.st_alive, ST_DTORS_FREE),
           alive_ok ? "PASS" : "FAIL");

    // drain 中: どの dtor の時点でも free counter は生存中と同じ (reclaim は drain 後).
    for (i = 0; i < 3; ++i) {
        long *s = g_rec.st_at_dtor[i];
        int ok = s[ST_OBJECT_FREE] == g_rec.st_alive[ST_OBJECT_FREE]
                 && s[ST_ENTRIES_FREE] == g_rec.st_alive[ST_ENTRIES_FREE]
                 && s[ST_DTORS_FREE] == g_rec.st_alive[ST_DTORS_FREE]
                 && s[ST_TCB_FREE] == g_rec.st_alive[ST_TCB_FREE]
                 && s[ST_RECLAIM_COMPLETED] == g_rec.st_alive[ST_RECLAIM_COMPLETED]
                 && s[ST_DRAIN_STARTED] == g_rec.st_alive[ST_DRAIN_STARTED] + 1
                 && s[ST_DRAIN_COMPLETED] == g_rec.st_alive[ST_DRAIN_COMPLETED]
                 && s[ST_DTOR_CALLS] == g_rec.st_alive[ST_DTOR_CALLS] + i + 1;
        printf("%c dtor: object_free_delta=%ld entries_free_delta=%ld dtors_free_delta=%ld tcb_free_delta=%ld "
               "drain_started=%ld drain_completed=%ld dtor_calls=%ld -> %s\n",
               g_rec.dtor_order[i],
               s[ST_OBJECT_FREE] - g_rec.st_alive[ST_OBJECT_FREE],
               s[ST_ENTRIES_FREE] - g_rec.st_alive[ST_ENTRIES_FREE],
               s[ST_DTORS_FREE] - g_rec.st_alive[ST_DTORS_FREE],
               s[ST_TCB_FREE] - g_rec.st_alive[ST_TCB_FREE],
               s[ST_DRAIN_STARTED] - g_rec.st_alive[ST_DRAIN_STARTED],
               s[ST_DRAIN_COMPLETED] - g_rec.st_alive[ST_DRAIN_COMPLETED],
               s[ST_DTOR_CALLS] - g_rec.st_alive[ST_DTOR_CALLS],
               ok ? "PASS" : "FAIL");
        if (!ok)
            no_reclaim_ok = 0;
    }
    printf("--- drain complete ---\n");

    // 既存 TLS への drain 中アクセス: 値が壊れていない.
    access_ok = g_rec.a_seen_in_B == 0xA && g_rec.x_seen_in_B == 7
                && g_rec.a_seen_in_C == 0xA && g_rec.x_seen_in_A == 7;
    printf("EXISTING_ACCESS_DURING_DRAIN: B saw a.v=0x%X x=%d, C saw a.v=0x%X, A saw x=%d -> %s\n",
           g_rec.a_seen_in_B, g_rec.x_seen_in_B, g_rec.a_seen_in_C, g_rec.x_seen_in_A,
           access_ok ? "PASS" : "FAIL");

    // join 後: 全 category で alloc == free, reclaim 1 回, fail-closed counter 0.
    reclaim_ok = d(st0, st1, ST_HOOK_DELIVERED) == 1 && d(st0, st1, ST_DRAIN_COMPLETED) == 1
                 && d(st0, st1, ST_RECLAIM_COMPLETED) == 1 && d(st0, st1, ST_DTOR_CALLS) == 3
                 && d(st0, st1, ST_TCB_ALLOC) == 1 && d(st0, st1, ST_TCB_FREE) == 1
                 && d(st0, st1, ST_OBJECT_ALLOC) == 4 && d(st0, st1, ST_OBJECT_FREE) == 4
                 && d(st0, st1, ST_ENTRIES_ALLOC) == 1 && d(st0, st1, ST_ENTRIES_FREE) == 1
                 && d(st0, st1, ST_DTORS_ALLOC) == 1 && d(st0, st1, ST_DTORS_FREE) == 1
                 && d(st0, st1, ST_SLOT_CLEAR_FAILURE) == 0
                 && d(st0, st1, ST_CROSS_THREAD_RECLAIM_SKIPPED) == 0;
    outstanding_ok = (st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE]) == 0
                     && (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE]) == 0
                     && (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]) == 0
                     && (st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE]) == 0;
    printf("storage reclaim: objects=%ld/%ld entries=%ld/%ld dtors=%ld/%ld\n",
           d(st0, st1, ST_OBJECT_ALLOC), d(st0, st1, ST_OBJECT_FREE),
           d(st0, st1, ST_ENTRIES_ALLOC), d(st0, st1, ST_ENTRIES_FREE),
           d(st0, st1, ST_DTORS_ALLOC), d(st0, st1, ST_DTORS_FREE));
    printf("TCB reclaim: %ld/%ld reclaim_completed=%ld slot_clear_failure=%ld cross_thread_skipped=%ld -> %s\n",
           d(st0, st1, ST_TCB_ALLOC), d(st0, st1, ST_TCB_FREE), d(st0, st1, ST_RECLAIM_COMPLETED),
           d(st0, st1, ST_SLOT_CLEAR_FAILURE), d(st0, st1, ST_CROSS_THREAD_RECLAIM_SKIPPED),
           reclaim_ok ? "PASS" : "FAIL");
    printf("OUTSTANDING_TCB=%ld OUTSTANDING_OBJECT=%ld OUTSTANDING_ENTRIES=%ld OUTSTANDING_DTORS=%ld -> %s\n",
           st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE], st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE],
           st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE], st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE],
           outstanding_ok ? "PASS" : "FAIL");

    if (!(order_ok && tid_ok && alive_ok && no_reclaim_ok && access_ok && reclaim_ok && outstanding_ok))
        fail = 1;
    if (__tcc_cpp_tls_n6_current_tcb() != 0) {
        printf("MAIN_HAS_UNEXPECTED_TCB\n");
        fail = 1;
    }
    if (fail) {
        printf("N6_04B_TLS_RECLAIM_NONTRIVIAL=FAIL\n");
        return 1;
    }
    printf("N6_04B_TLS_RECLAIM_NONTRIVIAL=PASS\n");
    printf("N6_04B_NONTRIVIAL_RECLAIM=PASS\n");
    printf("N6_04B_MULTI_OBJECT_RECLAIM=PASS\n");
    printf("N6_04B_NO_RECLAIM_BEFORE_DTOR_DRAIN=PASS\n");
    printf("N6_04B_EXISTING_ACCESS_DURING_DRAIN=PASS\n");
    printf("DTOR_ORDER=REVERSE_ACTUAL_CONSTRUCTION_ORDER\n");
    printf("DTOR_DRAIN_BEFORE_RECLAIM=PASS\n");
    return 0;
}
