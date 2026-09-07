// N6-04B Gate C: thread churn.  1000 worker threads in one process, 8 alive
// at a time (batches), each touching 2 non-trivial + 1 trivial thread_local.
//
// Authority は runtime 内部の deterministic accounting counter であり,
// process working set ではない (Windows heap は解放後も region を保持し得るので
// RSS が減らないことは leak の証拠にならない).  期待:
//   ALLOC_COUNT == FREE_COUNT              (全 category 合計)
//   OUTSTANDING_THREAD_TCB   == 0
//   OUTSTANDING_TLS_STORAGE  == 0          (object storage + entry array)
//   OUTSTANDING_DTOR_STORAGE == 0          (dtor registry array)
//   DTOR_CALLS == 2000, RECLAIM_COMPLETED == 1000, HOOK_DELIVERED == 1000
//   SLOT_CLEAR_FAILURE == 0, CROSS_THREAD_RECLAIM_SKIPPED == 0
//   各 dtor は自スレッド上 (mismatch 0)
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

#define THREADS 1000
#define BATCH 8

// per-thread結果は thread ごとの slot に書く (dtor は drain 中に g_slot を読む).
struct Slot {
    volatile DWORD tid;
    volatile int ctor_n;
    volatile int dtor_n;
    volatile int dtor_tid_mismatch;
    volatile int p_value_in_q_dtor;   // Q::~Q reads p.v (p constructed first) -> 1
};
static Slot g_slot_rec[THREADS];
thread_local int g_slot;
thread_local int x;

struct P { int v; P(); ~P(); };
struct Q { int v; Q(); ~Q(); };
thread_local P p;
thread_local Q q;

P::P() { v = 1; ++g_slot_rec[g_slot].ctor_n; }
P::~P()
{
    Slot *s = &g_slot_rec[g_slot];
    ++s->dtor_n;
    if (GetCurrentThreadId() != s->tid) ++s->dtor_tid_mismatch;
}
Q::Q() { v = 2; ++g_slot_rec[g_slot].ctor_n; }
Q::~Q()
{
    Slot *s = &g_slot_rec[g_slot];
    s->p_value_in_q_dtor = p.v;   // existing TLS access during drain
    ++s->dtor_n;
    if (GetCurrentThreadId() != s->tid) ++s->dtor_tid_mismatch;
}

static DWORD WINAPI worker(void *arg)
{
    int slot = (int)(INT_PTR)arg;
    g_slot = slot;
    g_slot_rec[slot].tid = GetCurrentThreadId();
    x = slot;
    p.v += 0;
    q.v += 0;
    return 0;
}

int main()
{
    long st0[ST_COUNT], st1[ST_COUNT];
    HANDLE h[BATCH];
    int i, j, fail = 0;
    long alloc_total, free_total, out_tcb, out_tls, out_dtor;
    int bad_slots = 0;

    memset(g_slot_rec, 0, sizeof g_slot_rec);
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    for (i = 0; i < THREADS; i += BATCH) {
        for (j = 0; j < BATCH && i + j < THREADS; ++j) {
            h[j] = CreateThread(NULL, 0, worker, (void *)(INT_PTR)(i + j), 0, NULL);
            if (!h[j]) { printf("CreateThread failed at %d\n", i + j); return 1; }
        }
        for (j = 0; j < BATCH && i + j < THREADS; ++j) {
            WaitForSingleObject(h[j], INFINITE);
            CloseHandle(h[j]);
        }
    }
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);

    for (i = 0; i < THREADS; ++i) {
        Slot *s = &g_slot_rec[i];
        if (s->ctor_n != 2 || s->dtor_n != 2 || s->dtor_tid_mismatch != 0 || s->p_value_in_q_dtor != 1) {
            if (bad_slots < 5)
                printf("SLOT %d: ctor=%d dtor=%d tid_mismatch=%d p_in_q=%d\n", i, s->ctor_n, s->dtor_n,
                       s->dtor_tid_mismatch, s->p_value_in_q_dtor);
            ++bad_slots;
        }
    }
    alloc_total = (st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC]) + (st1[ST_ENTRIES_ALLOC] - st0[ST_ENTRIES_ALLOC])
                  + (st1[ST_OBJECT_ALLOC] - st0[ST_OBJECT_ALLOC]) + (st1[ST_DTORS_ALLOC] - st0[ST_DTORS_ALLOC]);
    free_total = (st1[ST_TCB_FREE] - st0[ST_TCB_FREE]) + (st1[ST_ENTRIES_FREE] - st0[ST_ENTRIES_FREE])
                 + (st1[ST_OBJECT_FREE] - st0[ST_OBJECT_FREE]) + (st1[ST_DTORS_FREE] - st0[ST_DTORS_FREE]);
    out_tcb = st1[ST_TCB_ALLOC] - st1[ST_TCB_FREE];
    out_tls = (st1[ST_OBJECT_ALLOC] - st1[ST_OBJECT_FREE]) + (st1[ST_ENTRIES_ALLOC] - st1[ST_ENTRIES_FREE]);
    out_dtor = st1[ST_DTORS_ALLOC] - st1[ST_DTORS_FREE];

    printf("THREADS=%d BATCH=%d\n", THREADS, BATCH);
    printf("TCB=%ld/%ld ENTRIES=%ld/%ld OBJECTS=%ld/%ld DTORS=%ld/%ld\n",
           st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC], st1[ST_TCB_FREE] - st0[ST_TCB_FREE],
           st1[ST_ENTRIES_ALLOC] - st0[ST_ENTRIES_ALLOC], st1[ST_ENTRIES_FREE] - st0[ST_ENTRIES_FREE],
           st1[ST_OBJECT_ALLOC] - st0[ST_OBJECT_ALLOC], st1[ST_OBJECT_FREE] - st0[ST_OBJECT_FREE],
           st1[ST_DTORS_ALLOC] - st0[ST_DTORS_ALLOC], st1[ST_DTORS_FREE] - st0[ST_DTORS_FREE]);
    printf("HOOK_DELIVERED=%ld DRAIN_COMPLETED=%ld RECLAIM_COMPLETED=%ld DTOR_CALLS=%ld\n",
           st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED], st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED],
           st1[ST_RECLAIM_COMPLETED] - st0[ST_RECLAIM_COMPLETED], st1[ST_DTOR_CALLS] - st0[ST_DTOR_CALLS]);
    printf("SLOT_CLEAR_FAILURE=%ld CROSS_THREAD_RECLAIM_SKIPPED=%ld BAD_SLOTS=%d\n",
           st1[ST_SLOT_CLEAR_FAILURE] - st0[ST_SLOT_CLEAR_FAILURE],
           st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED], bad_slots);
    printf("ALLOC_COUNT=%ld\nFREE_COUNT=%ld\n", alloc_total, free_total);
    printf("OUTSTANDING_THREAD_TCB=%ld\nOUTSTANDING_TLS_STORAGE=%ld\nOUTSTANDING_DTOR_STORAGE=%ld\n",
           out_tcb, out_tls, out_dtor);

    if (alloc_total != free_total || out_tcb || out_tls || out_dtor || bad_slots
        || st1[ST_TCB_ALLOC] - st0[ST_TCB_ALLOC] != THREADS
        || st1[ST_OBJECT_ALLOC] - st0[ST_OBJECT_ALLOC] != THREADS * 4
        || st1[ST_DTOR_CALLS] - st0[ST_DTOR_CALLS] != THREADS * 2
        || st1[ST_RECLAIM_COMPLETED] - st0[ST_RECLAIM_COMPLETED] != THREADS
        || st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED] != THREADS
        || st1[ST_SLOT_CLEAR_FAILURE] - st0[ST_SLOT_CLEAR_FAILURE] != 0
        || st1[ST_CROSS_THREAD_RECLAIM_SKIPPED] - st0[ST_CROSS_THREAD_RECLAIM_SKIPPED] != 0
        || __tcc_cpp_tls_n6_current_tcb() != 0)
        fail = 1;
    if (fail) {
        printf("N6_04B_TLS_THREAD_CHURN=FAIL\n");
        return 1;
    }
    printf("N6_04B_TLS_THREAD_CHURN=PASS\n");
    printf("N6_04B_THREAD_CHURN=PASS\n");
    printf("ACCOUNTING_AUTHORITY=INTERNAL_COUNTERS_NOT_WORKING_SET\n");
    return 0;
}
