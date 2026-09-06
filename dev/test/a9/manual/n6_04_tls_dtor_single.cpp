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
//   join 後の TCB: hook_count=1, cleanup_state=2 (DONE), dtor_count=0 (drained)
// main thread は object に触らない (main の dtor は N6-05).
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
unsigned __tcc_cpp_tls_n6_registry_snapshot(void **objects, void **dtors, unsigned max);
void __tcc_cpp_tls_n6_tcb_inspect(void *tcb, DWORD *owner_tid, DWORD *cleanup_tid,
                                  unsigned *hook_count, int *cleanup_state,
                                  unsigned *dtor_count);
}

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
D::~D()
{
    ++g_rec[g_slot].dtor_only_count;
    g_rec[g_slot].dtor_only_tid = GetCurrentThreadId();
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
    DWORD owner = 0, cleanup_tid = 0;
    unsigned hooks = 0, dtors = 9;
    int state = 0;
    int ok;
    __tcc_cpp_tls_n6_tcb_inspect(r->tcb, &owner, &cleanup_tid, &hooks, &state, &dtors);
    ok = r->ctor_count == 1 && r->dtor_count == 1 && r->dtor_tid == r->tid
         && r->dtor_only_count == 1 && r->dtor_only_tid == r->tid
         && r->value_seen == 103 && r->registered_while_alive == 2
         && hooks == 1 && state == 2 && dtors == 0
         && owner == r->tid && cleanup_tid == r->tid;
    printf("%s: ctor=%d dtor=%d dtor_tid_eq_owner=%s dtor_only=%d dtor_only_tid_eq_owner=%s "
           "value=%d registered_alive=%u joined{hook_count=%u state=%d dtor_count=%u} -> %s\n",
           name, r->ctor_count, r->dtor_count, r->dtor_tid == r->tid ? "YES" : "NO",
           r->dtor_only_count, r->dtor_only_tid == r->tid ? "YES" : "NO",
           r->value_seen, r->registered_while_alive, hooks, state, dtors,
           ok ? "PASS" : "FAIL");
    return ok;
}

int main()
{
    HANDLE h;
    int i;
    int fail = 0;

    memset(g_rec, 0, sizeof g_rec);
    g_slot = 0;
    // slot 0 は main 用 (main は object に触らないので 0 のまま).

    for (i = 1; i <= 3; ++i) {
        h = CreateThread(NULL, 0, worker, (void *)(INT_PTR)i, 0, NULL);
        if (!h) { printf("CreateThread failed\n"); return 1; }
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
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
