// N6-04 Authority: LIFO destructor drain in reverse ACTUAL construction order,
// two concurrent workers, destructor runs on the owning thread.
//
// 宣言順は A, B, C. worker 1 は A, C, B の順に, worker 2 は B, A, C の順に
// 初回 touch する (実際の構築順 != 宣言順).  期待:
//   worker 1: CTOR_ORDER=A,C,B  DTOR_ORDER=B,C,A
//   worker 2: CTOR_ORDER=B,A,C  DTOR_ORDER=C,A,B
// 両 worker は構築済み TLS を持ったまま同時生存し, 同じ event で同時に exit する
// (drain も並行).  各 dtor は GetCurrentThreadId() を記録し, owner tid と照合する.
//
// 測定:
//   N6_04_LIFO                       dtor 順 == 構築順の逆
//   N6_04_ACTUAL_CONSTRUCTION_ORDER  宣言順ではなく実構築順を基準にしている
//   N6_04_CONCURRENT_THREADS         A/B 同時生存 + 同時 exit で各 thread exactly once
//   N6_04_DTOR_RUNS_ON_OWNER_THREAD  A_DTOR_THREAD_ID=A_THREAD_ID, B_... = B_...
//   registry snapshot (生存中) == 実構築順  (REGISTRATION_ORDER=ACTUAL_CONSTRUCTION_ORDER)
//   hook state は最初の dtor 内 (drain 中, TCB 生存) で観測: hook_count=1,
//     cleanup_state=1, cleanup_tid == owner_tid == worker tid, dtor_count=2
//     (3 登録, 1 pop 済み = pop-before-callback).  N6-04B 以降 TCB は thread exit で
//     解放されるので join 後は TCB を読まず accounting counter (TCB_FREE +2) を見る.
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
unsigned __tcc_cpp_tls_n6_registry_snapshot(void **objects, void **dtors, unsigned max);
void __tcc_cpp_tls_n6_tcb_inspect(void *tcb, DWORD *owner_tid, DWORD *cleanup_tid,
                                  unsigned *hook_count, int *cleanup_state,
                                  unsigned *dtor_count);
unsigned __tcc_cpp_tls_n6_stats(long *out, unsigned max);
}
enum { ST_TCB_ALLOC, ST_TCB_FREE, ST_ENTRIES_ALLOC, ST_ENTRIES_FREE, ST_OBJECT_ALLOC,
       ST_OBJECT_FREE, ST_DTORS_ALLOC, ST_DTORS_FREE, ST_HOOK_DELIVERED, ST_DRAIN_STARTED,
       ST_DRAIN_COMPLETED, ST_RECLAIM_COMPLETED, ST_SLOT_CLEAR_FAILURE,
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT };

struct Record {
    volatile DWORD tid;
    void *volatile tcb;
    char ctor_order[8];
    volatile int ctor_n;
    char dtor_order[8];
    volatile int dtor_n;
    DWORD dtor_tid[8];
    void *addr_a, *addr_b, *addr_c;
    void *snapshot[8];
    unsigned snapshot_n;
    volatile int ready;
    HANDLE go;
    // 最初の dtor 内で読んだ hook state (drain 中).
    void *volatile tcb_in_drain;
    volatile DWORD owner_in_drain;
    volatile DWORD cleanup_tid_in_drain;
    volatile unsigned hooks_in_drain;
    volatile int state_in_drain;
    volatile unsigned dtor_count_in_drain;
};
static Record g_rec[3];

// drain 中に dtor が自分の record を選ぶための trivial TLS (既構築).
thread_local int g_slot;

static void log_ctor(char tag)
{
    Record *r = &g_rec[g_slot];
    if (r->ctor_n < 8)
        r->ctor_order[r->ctor_n] = tag;
    ++r->ctor_n;
}
static void log_dtor(char tag)
{
    Record *r = &g_rec[g_slot];
    if (r->dtor_n == 0) {
        // 最初の dtor: TCB はまだ生存 (drain 中). hook state をここで観測する.
        DWORD owner = 0, cleanup_tid = 0;
        unsigned hooks = 0, dtors = 9;
        int state = 0;
        r->tcb_in_drain = __tcc_cpp_tls_n6_current_tcb();
        __tcc_cpp_tls_n6_tcb_inspect(r->tcb_in_drain, &owner, &cleanup_tid, &hooks, &state, &dtors);
        r->owner_in_drain = owner;
        r->cleanup_tid_in_drain = cleanup_tid;
        r->hooks_in_drain = hooks;
        r->state_in_drain = state;
        r->dtor_count_in_drain = dtors;
    }
    if (r->dtor_n < 8) {
        r->dtor_order[r->dtor_n] = tag;
        r->dtor_tid[r->dtor_n] = GetCurrentThreadId();
    }
    ++r->dtor_n;
}

struct A { int v; A(); ~A(); };
struct B { int v; B(); ~B(); };
struct C { int v; C(); ~C(); };

thread_local A a;
thread_local B b;
thread_local C c;

A::A() { v = 0xA; log_ctor('A'); }
A::~A() { log_dtor('A'); }
B::B() { v = 0xB; log_ctor('B'); }
B::~B() { log_dtor('B'); }
C::C() { v = 0xC; log_ctor('C'); }
C::~C() { log_dtor('C'); }

static DWORD WINAPI worker(void *p)
{
    int slot = (int)(INT_PTR)p;
    Record *r = &g_rec[slot];
    g_slot = slot;
    r->tid = GetCurrentThreadId();
    if (slot == 1) {
        r->addr_a = &a; r->addr_c = &c; r->addr_b = &b;
    } else {
        r->addr_b = &b; r->addr_a = &a; r->addr_c = &c;
    }
    // 再 touch しても registry / ctor は増えない.
    a.v += 1; b.v += 1; c.v += 1;
    r->snapshot_n = __tcc_cpp_tls_n6_registry_snapshot(r->snapshot, 0, 8);
    r->tcb = __tcc_cpp_tls_n6_current_tcb();
    r->ready = 1;
    WaitForSingleObject(r->go, INFINITE);
    return 0;
}

static int check(const char *name, Record *r, const char *expect_ctor, const char *expect_dtor)
{
    int i, ok, tid_ok = 1, snap_ok, hook_ok;
    void *expect_snap[3];

    // join 後の TCB は解放済み (N6-04B). drain 中に読んだ値で hook state を検証.
    hook_ok = r->tcb != 0 && r->tcb_in_drain == r->tcb
              && r->hooks_in_drain == 1 && r->state_in_drain == 1 && r->dtor_count_in_drain == 2
              && r->owner_in_drain == r->tid && r->cleanup_tid_in_drain == r->tid;
    r->ctor_order[3] = 0;
    r->dtor_order[3] = 0;
    for (i = 0; i < 3; ++i)
        if (r->dtor_tid[i] != r->tid)
            tid_ok = 0;
    // 生存中の registry snapshot は実構築順の object address 列.
    for (i = 0; i < 3; ++i) {
        void *want = expect_ctor[i] == 'A' ? r->addr_a : expect_ctor[i] == 'B' ? r->addr_b : r->addr_c;
        expect_snap[i] = want;
    }
    snap_ok = r->snapshot_n == 3 && r->snapshot[0] == expect_snap[0]
              && r->snapshot[1] == expect_snap[1] && r->snapshot[2] == expect_snap[2];
    ok = r->ctor_n == 3 && r->dtor_n == 3
         && strcmp(r->ctor_order, expect_ctor) == 0
         && strcmp(r->dtor_order, expect_dtor) == 0
         && tid_ok && snap_ok && hook_ok;
    printf("%s: CTOR_ORDER=%c,%c,%c DTOR_ORDER=%c,%c,%c (expected ctor %c,%c,%c dtor %c,%c,%c) "
           "ctor_n=%d dtor_n=%d dtor_tids_eq_owner=%s registry_snapshot_eq_ctor_order=%s "
           "in_drain{same_tcb=%s hook_count=%u state=%d dtor_count=%u owner_eq=%s cleanup_tid_eq=%s} -> %s\n",
           name, r->ctor_order[0], r->ctor_order[1], r->ctor_order[2],
           r->dtor_order[0], r->dtor_order[1], r->dtor_order[2],
           expect_ctor[0], expect_ctor[1], expect_ctor[2],
           expect_dtor[0], expect_dtor[1], expect_dtor[2],
           r->ctor_n, r->dtor_n, tid_ok ? "YES" : "NO", snap_ok ? "YES" : "NO",
           r->tcb_in_drain == r->tcb ? "YES" : "NO",
           r->hooks_in_drain, r->state_in_drain, r->dtor_count_in_drain,
           r->owner_in_drain == r->tid ? "YES" : "NO",
           r->cleanup_tid_in_drain == r->tid ? "YES" : "NO",
           ok ? "PASS" : "FAIL");
    return ok;
}

int main()
{
    HANDLE h1, h2, go;
    int fail = 0;
    long st0[ST_COUNT], st1[ST_COUNT];

    memset(g_rec, 0, sizeof g_rec);
    g_slot = 0;
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    go = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_rec[1].go = g_rec[2].go = go;
    h1 = CreateThread(NULL, 0, worker, (void *)(INT_PTR)1, 0, NULL);
    h2 = CreateThread(NULL, 0, worker, (void *)(INT_PTR)2, 0, NULL);
    if (!h1 || !h2) { printf("CreateThread failed\n"); return 1; }
    while (!g_rec[1].ready || !g_rec[2].ready)
        Sleep(1);
    // 両 worker 生存中: TCB / storage は分離, dtor はまだ 0.
    printf("BOTH_ALIVE: tcb_isolated=%s storage_isolated=%s dtor_n=%d,%d\n",
           g_rec[1].tcb != g_rec[2].tcb ? "YES" : "NO",
           (g_rec[1].addr_a != g_rec[2].addr_a && g_rec[1].addr_b != g_rec[2].addr_b
            && g_rec[1].addr_c != g_rec[2].addr_c) ? "YES" : "NO",
           g_rec[1].dtor_n, g_rec[2].dtor_n);
    if (g_rec[1].tcb == g_rec[2].tcb || g_rec[1].addr_a == g_rec[2].addr_a
        || g_rec[1].dtor_n != 0 || g_rec[2].dtor_n != 0)
        fail = 1;
    // 同時に exit させる (drain 並行).
    SetEvent(go);
    WaitForSingleObject(h1, INFINITE);
    WaitForSingleObject(h2, INFINITE);
    CloseHandle(h1);
    CloseHandle(h2);
    CloseHandle(go);

    if (!check("N6_04_LIFO_WORKER_A", &g_rec[1], "ACB", "BCA")) fail = 1;
    if (!check("N6_04_LIFO_WORKER_B", &g_rec[2], "BAC", "CAB")) fail = 1;
    printf("A_DTOR_THREAD_ID=%s\nB_DTOR_THREAD_ID=%s\n",
           (g_rec[1].dtor_tid[0] == g_rec[1].tid && g_rec[1].dtor_tid[1] == g_rec[1].tid
            && g_rec[1].dtor_tid[2] == g_rec[1].tid) ? "A_THREAD_ID" : "MISMATCH",
           (g_rec[2].dtor_tid[0] == g_rec[2].tid && g_rec[2].dtor_tid[1] == g_rec[2].tid
            && g_rec[2].dtor_tid[2] == g_rec[2].tid) ? "B_THREAD_ID" : "MISMATCH");
    printf("MAIN_SLOT_CTOR_N=%d MAIN_SLOT_DTOR_N=%d\n", g_rec[0].ctor_n, g_rec[0].dtor_n);
    if (g_rec[0].ctor_n != 0 || g_rec[0].dtor_n != 0)
        fail = 1;
    // join 後: 2 worker 分の hook / drain / TCB 解放が counter に現れる.
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    printf("AFTER_JOIN: hooks=%ld drains=%ld dtor_calls=%ld tcb_free=%ld (expected 2,2,6,2)\n",
           st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED],
           st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED],
           st1[ST_DTOR_CALLS] - st0[ST_DTOR_CALLS],
           st1[ST_TCB_FREE] - st0[ST_TCB_FREE]);
    if (st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED] != 2
        || st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED] != 2
        || st1[ST_DTOR_CALLS] - st0[ST_DTOR_CALLS] != 6
        || st1[ST_TCB_FREE] - st0[ST_TCB_FREE] != 2)
        fail = 1;

    if (fail) {
        printf("N6_04_TLS_DTOR_LIFO=FAIL\n");
        return 1;
    }
    printf("N6_04_TLS_DTOR_LIFO=PASS\n");
    printf("N6_04_LIFO=PASS\n");
    printf("N6_04_ACTUAL_CONSTRUCTION_ORDER=PASS\n");
    printf("N6_04_CONCURRENT_THREADS=PASS\n");
    printf("N6_04_DTOR_RUNS_ON_OWNER_THREAD=PASS\n");
    printf("REGISTRATION_ORDER=ACTUAL_CONSTRUCTION_ORDER\n");
    printf("DTOR_ORDER=REVERSE_ACTUAL_CONSTRUCTION_ORDER\n");
    printf("DTOR_REGISTRY_OWNER=PER_THREAD_TCB\n");
    return 0;
}
