// N6-04B concurrent gate: two workers alive at once, exit at once, each drains
// and reclaims only its own storage on its own thread.
//
//   Thread A: touch A, C, B  -> dtor B, C, A
//   Thread B: touch B, A, C  -> dtor C, A, B
//
// 期待:
//   A destructor = B,C,A / B destructor = C,A,B (N6-04 と同じ, 実構築順の逆)
//   dtor はすべて owner thread 上 (dtor 内 GetCurrentThreadId() == owner tid)
//   join 後: TCB 2/2, objects 8/8 (g_slot + A + B + C x 2), entries 2/2,
//            dtors 2/2, reclaim_completed=2
//   cross-thread free = 0: runtime は cleanup_tid != owner_tid の場合 reclaim を
//   スキップして counter CROSS_THREAD_RECLAIM_SKIPPED を立てる. それが 0 かつ
//   RECLAIM_COMPLETED == 2 なら両 reclaim は owner thread 上で行われた
//   (ALLOCATING_THREAD == DESTRUCTOR_THREAD == RECLAIM_THREAD).
//   各 dtor 内で読んだ current TCB の owner_tid == 自 tid (TCB は自スレッド所有).
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
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

struct Record {
    volatile DWORD tid;
    void *volatile tcb;
    char ctor_order[8];
    volatile int ctor_n;
    char dtor_order[8];
    volatile int dtor_n;
    DWORD dtor_tid[8];
    DWORD owner_seen[8];      // owner_tid of the current TCB inside each dtor
    DWORD cleanup_tid_seen[8];
    volatile int ready;
    HANDLE go;
};
static Record g_rec[3];
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
        DWORD owner = 0, ct = 0;
        r->dtor_order[r->dtor_n] = tag;
        r->dtor_tid[r->dtor_n] = GetCurrentThreadId();
        __tcc_cpp_tls_n6_tcb_inspect(__tcc_cpp_tls_n6_current_tcb(), &owner, &ct, 0, 0, 0);
        r->owner_seen[r->dtor_n] = owner;
        r->cleanup_tid_seen[r->dtor_n] = ct;
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
    if (slot == 1) { a.v += 0; c.v += 0; b.v += 0; }
    else           { b.v += 0; a.v += 0; c.v += 0; }
    r->tcb = __tcc_cpp_tls_n6_current_tcb();
    r->ready = 1;
    WaitForSingleObject(r->go, INFINITE);
    return 0;
}

static long d(long *a0, long *b0, int i) { return b0[i] - a0[i]; }

static int check(const char *name, Record *r, const char *ec, const char *ed)
{
    int i, ok, tid_ok = 1;
    r->ctor_order[3] = 0;
    r->dtor_order[3] = 0;
    for (i = 0; i < 3; ++i)
        if (r->dtor_tid[i] != r->tid || r->owner_seen[i] != r->tid || r->cleanup_tid_seen[i] != r->tid)
            tid_ok = 0;
    ok = r->ctor_n == 3 && r->dtor_n == 3 && strcmp(r->ctor_order, ec) == 0
         && strcmp(r->dtor_order, ed) == 0 && tid_ok;
    printf("%s: CTOR_ORDER=%c,%c,%c DTOR_ORDER=%c,%c,%c (expected %c,%c,%c) "
           "dtor_tid==owner_tid==cleanup_tid==thread=%s -> %s\n",
           name, r->ctor_order[0], r->ctor_order[1], r->ctor_order[2],
           r->dtor_order[0], r->dtor_order[1], r->dtor_order[2], ed[0], ed[1], ed[2],
           tid_ok ? "YES" : "NO", ok ? "PASS" : "FAIL");
    return ok;
}

int main()
{
    HANDLE h1, h2, go;
    long st0[ST_COUNT], st1[ST_COUNT];
    int fail = 0, acc_ok;

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
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    printf("BOTH_ALIVE: tcb_isolated=%s tcb_alloc=%ld objects=%ld frees=%ld dtors=%d,%d\n",
           g_rec[1].tcb != g_rec[2].tcb ? "YES" : "NO",
           d(st0, st1, ST_TCB_ALLOC), d(st0, st1, ST_OBJECT_ALLOC),
           d(st0, st1, ST_TCB_FREE) + d(st0, st1, ST_OBJECT_FREE),
           g_rec[1].dtor_n, g_rec[2].dtor_n);
    // 1 worker = g_slot (trivial) + A + B + C = 4 object.
    if (g_rec[1].tcb == g_rec[2].tcb || d(st0, st1, ST_TCB_ALLOC) != 2
        || d(st0, st1, ST_OBJECT_ALLOC) != 8 || d(st0, st1, ST_TCB_FREE) != 0
        || d(st0, st1, ST_OBJECT_FREE) != 0 || g_rec[1].dtor_n || g_rec[2].dtor_n)
        fail = 1;
    // 同時 exit: drain / reclaim が並行に走る.
    SetEvent(go);
    WaitForSingleObject(h1, INFINITE);
    WaitForSingleObject(h2, INFINITE);
    CloseHandle(h1);
    CloseHandle(h2);
    CloseHandle(go);
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);

    if (!check("N6_04B_CONCURRENT_WORKER_A", &g_rec[1], "ACB", "BCA")) fail = 1;
    if (!check("N6_04B_CONCURRENT_WORKER_B", &g_rec[2], "BAC", "CAB")) fail = 1;
    acc_ok = d(st0, st1, ST_HOOK_DELIVERED) == 2 && d(st0, st1, ST_DRAIN_COMPLETED) == 2
             && d(st0, st1, ST_RECLAIM_COMPLETED) == 2 && d(st0, st1, ST_DTOR_CALLS) == 6
             && d(st0, st1, ST_TCB_ALLOC) == 2 && d(st0, st1, ST_TCB_FREE) == 2
             && d(st0, st1, ST_OBJECT_ALLOC) == 8 && d(st0, st1, ST_OBJECT_FREE) == 8
             && d(st0, st1, ST_ENTRIES_ALLOC) == 2 && d(st0, st1, ST_ENTRIES_FREE) == 2
             && d(st0, st1, ST_DTORS_ALLOC) == 2 && d(st0, st1, ST_DTORS_FREE) == 2
             && d(st0, st1, ST_CROSS_THREAD_RECLAIM_SKIPPED) == 0
             && d(st0, st1, ST_SLOT_CLEAR_FAILURE) == 0
             && g_rec[0].ctor_n == 0 && g_rec[0].dtor_n == 0;
    printf("AFTER_JOIN: hooks=%ld drains=%ld reclaims=%ld dtor_calls=%ld tcb=%ld/%ld objects=%ld/%ld "
           "entries=%ld/%ld dtors=%ld/%ld cross_thread_free=%ld slot_clear_failure=%ld -> %s\n",
           d(st0, st1, ST_HOOK_DELIVERED), d(st0, st1, ST_DRAIN_COMPLETED),
           d(st0, st1, ST_RECLAIM_COMPLETED), d(st0, st1, ST_DTOR_CALLS),
           d(st0, st1, ST_TCB_ALLOC), d(st0, st1, ST_TCB_FREE),
           d(st0, st1, ST_OBJECT_ALLOC), d(st0, st1, ST_OBJECT_FREE),
           d(st0, st1, ST_ENTRIES_ALLOC), d(st0, st1, ST_ENTRIES_FREE),
           d(st0, st1, ST_DTORS_ALLOC), d(st0, st1, ST_DTORS_FREE),
           d(st0, st1, ST_CROSS_THREAD_RECLAIM_SKIPPED), d(st0, st1, ST_SLOT_CLEAR_FAILURE),
           acc_ok ? "PASS" : "FAIL");
    if (!acc_ok)
        fail = 1;
    if (fail) {
        printf("N6_04B_TLS_RECLAIM_CONCURRENT=FAIL\n");
        return 1;
    }
    printf("N6_04B_TLS_RECLAIM_CONCURRENT=PASS\n");
    printf("N6_04B_CONCURRENT_RECLAIM=PASS\n");
    printf("A_DTOR_ORDER=B,C,A\nB_DTOR_ORDER=C,A,B\n");
    printf("A_ALLOCATIONS_FULLY_RECLAIMED=YES\nB_ALLOCATIONS_FULLY_RECLAIMED=YES\n");
    printf("CROSS_THREAD_FREE=0\n");
    printf("OWNER_THREAD_RECLAIM=PASS (ALLOCATING_THREAD==DESTRUCTOR_THREAD==RECLAIM_THREAD)\n");
    return 0;
}
