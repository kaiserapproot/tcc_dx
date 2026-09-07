// N6-04 Authority: single thread_local object with a user destructor.
//
// 初めて frontend が受理する形 (NONTRIVIAL_DTOR_FRONTEND_ACCEPTANCE=YES):
//   struct P { P(); ~P(); };  thread_local P object;
//
// 測定 (worker thread, PE TLS callback DLL_THREAD_DETACH で drain):
//   N6_04_SINGLE_OBJECT_DTOR       worker が object を 3 回 touch -> ctor 1 / dtor 1
//   N6_04_DTOR_EXACTLY_ONCE        3 回 touch でも registration 1, dtor 1
//   N6_04_DTOR_RUNS_ON_OWNER_THREAD dtor 内の GetCurrentThreadId() == worker tid
//   N6_04_UNACCESSED_NO_CTOR_NO_DTOR 未アクセスの `thread_local P cold;` は 0 / 0
//   N6_04_DTOR_ONLY_CLASS          ctor を持たず dtor だけの class も dtor 1
//   N6_04_PER_THREAD_DTOR          worker を 3 本逐次作成 -> 各 thread で exactly once
//   drain 中の TCB (P::~P 内で観測): hook_count=1, cleanup_state=1 (DRAINING),
//     cleanup_tid == owner_tid == worker tid, dtor_count=1 (P は pop 済み, D が残り)
//   join 後 (N6-04B で TCB は解放済みなので TCB は読まない): accounting counter で
//     HOOK_DELIVERED / DRAIN_COMPLETED / TCB_FREE が worker ごとに +1
// main thread は object に触らない (main の dtor は N6-05).
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
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_POST_FINALIZE_TCB,
       ST_POST_FINALIZE_OBJECT, ST_COUNT };

// worker ごとの記録先. dtor は drain 中に thread_local int g_slot (既構築, trivial)
// を読んで自分の record を選ぶ (EXISTING_TLS_ACCESS_DURING_DRAIN の実使用).
struct Record {
    volatile DWORD tid;
    void *volatile tcb;
    volatile int ctor_count;
    volatile int dtor_count;
    volatile DWORD dtor_tid;
    volatile int dtor_only_count;
    volatile DWORD dtor_only_tid;
    volatile int value_seen;
    volatile unsigned registered_while_alive;
    volatile int ready;
    HANDLE go;
    // P::~P 内 (drain 中, TCB 生存) で読んだ hook state.
    void *volatile tcb_in_drain;
    volatile DWORD owner_in_drain;
    volatile DWORD cleanup_tid_in_drain;
    volatile unsigned hooks_in_drain;
    volatile int state_in_drain;
    volatile unsigned dtor_count_in_drain;
};
static Record g_rec[4];
static volatile int g_cold_ctor;
static volatile int g_cold_dtor;

thread_local int g_slot;

struct P {
    int value;
    P();
    ~P();
};

// ctor なし, user dtor だけの class (storage は runtime が zero-fill).
struct D {
    int value;
    ~D();
};

struct Cold {
    int value;
    Cold();
    ~Cold();
};

thread_local P object;
thread_local D dtor_only;
thread_local Cold cold;

P::P() { value = 100; ++g_rec[g_slot].ctor_count; }
P::~P()
{
    ++g_rec[g_slot].dtor_count;
    g_rec[g_slot].dtor_tid = GetCurrentThreadId();
}
// D は P の後に構築されるので LIFO で最初に drain される. ここで TCB を観測する:
// registry は [P, D] -> D を pop してから callback なので dtor_count は 1.
D::~D()
{
    Record *r = &g_rec[g_slot];
    DWORD owner = 0, cleanup_tid = 0;
    unsigned hooks = 0, dtors = 9;
    int state = 0;
    ++r->dtor_only_count;
    r->dtor_only_tid = GetCurrentThreadId();
    r->tcb_in_drain = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_tcb_inspect(r->tcb_in_drain, &owner, &cleanup_tid, &hooks, &state, &dtors);
    r->owner_in_drain = owner;
    r->cleanup_tid_in_drain = cleanup_tid;
    r->hooks_in_drain = hooks;
    r->state_in_drain = state;
    r->dtor_count_in_drain = dtors;
}
Cold::Cold() { value = 1; ++g_cold_ctor; }
Cold::~Cold() { ++g_cold_dtor; }

static DWORD WINAPI worker(void *p)
{
    int slot = (int)(INT_PTR)p;
    Record *r = &g_rec[slot];
    g_slot = slot;
    r->tid = GetCurrentThreadId();
    object.value += 1;
    object.value += 1;
    object.value += 1;
    r->value_seen = object.value;
    dtor_only.value = 5;
    // 生存中: registry には object と dtor_only の 2 entry だけ (cold は未登録).
    r->registered_while_alive = __tcc_cpp_tls_n6_registry_snapshot(0, 0, 0);
    r->tcb = __tcc_cpp_tls_n6_current_tcb();
    r->ready = 1;
    if (r->go)
        WaitForSingleObject(r->go, INFINITE);
    return 0;
}

static int check(const char *name, Record *r)
{
    int ok;
    // TCB は N6-04B で thread exit 時に解放されるため join 後には読まない.
    // hook state は D::~D (drain 中) で観測した値を検証する.
    ok = r->ctor_count == 1 && r->dtor_count == 1 && r->dtor_tid == r->tid
         && r->dtor_only_count == 1 && r->dtor_only_tid == r->tid
         && r->value_seen == 103 && r->registered_while_alive == 2
         && r->tcb_in_drain == r->tcb && r->tcb != 0
         && r->hooks_in_drain == 1 && r->state_in_drain == 1 && r->dtor_count_in_drain == 1
         && r->owner_in_drain == r->tid && r->cleanup_tid_in_drain == r->tid;
    printf("%s: ctor=%d dtor=%d dtor_tid_eq_owner=%s dtor_only=%d dtor_only_tid_eq_owner=%s "
           "value=%d registered_alive=%u in_drain{same_tcb=%s hook_count=%u state=%d dtor_count=%u "
           "owner_eq=%s cleanup_tid_eq=%s} -> %s\n",
           name, r->ctor_count, r->dtor_count, r->dtor_tid == r->tid ? "YES" : "NO",
           r->dtor_only_count, r->dtor_only_tid == r->tid ? "YES" : "NO",
           r->value_seen, r->registered_while_alive,
           r->tcb_in_drain == r->tcb ? "YES" : "NO",
           r->hooks_in_drain, r->state_in_drain, r->dtor_count_in_drain,
           r->owner_in_drain == r->tid ? "YES" : "NO",
           r->cleanup_tid_in_drain == r->tid ? "YES" : "NO",
           ok ? "PASS" : "FAIL");
    return ok;
}

int main()
{
    HANDLE h;
    int i;
    int fail = 0;
    long st0[ST_COUNT], st1[ST_COUNT];

    memset(g_rec, 0, sizeof g_rec);
    g_slot = 0;
    // slot 0 は main 用 (main は object に触らないので 0 のまま).
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);

    for (i = 1; i <= 3; ++i) {
        h = CreateThread(NULL, 0, worker, (void *)(INT_PTR)i, 0, NULL);
        if (!h) { printf("CreateThread failed\n"); return 1; }
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        // 逐次 worker: 1 本 join するごとに hook / drain / TCB 解放が +1.
        __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
        printf("W%d_JOINED: hooks=%ld drains=%ld tcb_free=%ld (expected %d each)\n", i,
               st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED],
               st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED],
               st1[ST_TCB_FREE] - st0[ST_TCB_FREE], i);
        if (st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED] != i
            || st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED] != i
            || st1[ST_TCB_FREE] - st0[ST_TCB_FREE] != i)
            fail = 1;
    }
    if (!check("N6_04_SINGLE_OBJECT_DTOR_W1", &g_rec[1])) fail = 1;
    if (!check("N6_04_PER_THREAD_DTOR_W2", &g_rec[2])) fail = 1;
    if (!check("N6_04_PER_THREAD_DTOR_W3", &g_rec[3])) fail = 1;

    printf("MAIN_SLOT_CTOR=%d MAIN_SLOT_DTOR=%d (main never touched object)\n",
           g_rec[0].ctor_count, g_rec[0].dtor_count);
    if (g_rec[0].ctor_count != 0 || g_rec[0].dtor_count != 0)
        fail = 1;
    printf("COLD_CTOR_COUNT=%d COLD_DTOR_COUNT=%d\n", g_cold_ctor, g_cold_dtor);
    if (g_cold_ctor != 0 || g_cold_dtor != 0)
        fail = 1;

    if (fail) {
        printf("N6_04_TLS_DTOR_SINGLE=FAIL\n");
        return 1;
    }
    printf("N6_04_TLS_DTOR_SINGLE=PASS\n");
    printf("N6_04_SINGLE_OBJECT_DTOR=PASS\n");
    printf("N6_04_DTOR_EXACTLY_ONCE=PASS\n");
    printf("N6_04_PER_THREAD_DTOR=PASS\n");
    printf("N6_04_DTOR_RUNS_ON_OWNER_THREAD=PASS\n");
    printf("N6_04_UNACCESSED_NO_CTOR_NO_DTOR=PASS\n");
    printf("N6_04_DTOR_ONLY_CLASS=PASS\n");
    printf("NONTRIVIAL_DTOR_FRONTEND_ACCEPTANCE=YES\n");
    printf("DTOR_EXECUTION_CONTEXT=PE_TLS_CALLBACK_DLL_THREAD_DETACH\n");
    printf("MAIN_THREAD_TLS_DTOR=DEFERRED_TO_N6_05\n");
    return 0;
}
