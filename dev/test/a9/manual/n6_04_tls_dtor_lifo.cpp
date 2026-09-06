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
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
unsigned __tcc_cpp_tls_n6_registry_snapshot(void **objects, void **dtors, unsigned max);
void __tcc_cpp_tls_n6_tcb_inspect(void *tcb, DWORD *owner_tid, DWORD *cleanup_tid,
                                  unsigned *hook_count, int *cleanup_state,
                                  unsigned *dtor_count);
}

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
    DWORD owner = 0, cleanup_tid = 0;
    unsigned hooks = 0, dtors = 9;
    int state = 0;
    int i, ok, tid_ok = 1, snap_ok;
    void *expect_snap[3];

    __tcc_cpp_tls_n6_tcb_inspect(r->tcb, &owner, &cleanup_tid, &hooks, &state, &dtors);
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
         && tid_ok && snap_ok
         && hooks == 1 && state == 2 && dtors == 0
         && owner == r->tid && cleanup_tid == r->tid;
    printf("%s: CTOR_ORDER=%c,%c,%c DTOR_ORDER=%c,%c,%c (expected ctor %c,%c,%c dtor %c,%c,%c) "
           "ctor_n=%d dtor_n=%d dtor_tids_eq_owner=%s registry_snapshot_eq_ctor_order=%s "
           "joined{hook_count=%u state=%d dtor_count=%u} -> %s\n",
           name, r->ctor_order[0], r->ctor_order[1], r->ctor_order[2],
           r->dtor_order[0], r->dtor_order[1], r->dtor_order[2],
           expect_ctor[0], expect_ctor[1], expect_ctor[2],
           expect_dtor[0], expect_dtor[1], expect_dtor[2],
           r->ctor_n, r->dtor_n, tid_ok ? "YES" : "NO", snap_ok ? "YES" : "NO",
           hooks, state, dtors, ok ? "PASS" : "FAIL");
    return ok;
}

int main()
{
    HANDLE h1, h2, go;
    int fail = 0;

    memset(g_rec, 0, sizeof g_rec);
    g_slot = 0;
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
