// N6-03 Authority: per-thread destructor registry foundation (raw runtime ABI).
//
// The registry is exercised directly through the runtime ABI
// `__tcc_cpp_tls_addr(desc, size, ctor, dtor)` from two concurrent workers
// (independent of the frontend), and read back with the N6 inspection API.
//
// Measured:
//   N6_03_REGISTRATION_CONSTRUCTION_ORDER  A, C, B constructed -> [A, C, B]
//   N6_03_REGISTRATION_EXACTLY_ONCE        second access to A adds nothing
//   N6_03_PER_THREAD_REGISTRY_ISOLATION    worker A's objects never appear in
//                                          worker B's registry and vice versa
//   NO_REGISTRATION_WHILE_INITIALIZING     C's ctor sees only [A] registered
//   TRIVIAL_OBJECT_NOT_REGISTERED          dtor=NULL object (T) is absent
//   while both workers are alive no dtor callback has run
// N6-04 (drain implemented): after join each worker's TCB shows hook_count=1,
//   cleanup_tid=its own tid, cleanup_state=2 (DONE), dtor_count=0 (drained),
//   and the callbacks ran exactly once per worker in LIFO order B, C, A on
//   the owning thread (the callback records the slot stored in the object).
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_addr(int *descriptor, unsigned size, void (*ctor)(void *),
                         void (*dtor)(void *));
unsigned __tcc_cpp_tls_n6_registry_snapshot(void **objects, void **dtors,
                                            unsigned max);
void *__tcc_cpp_tls_n6_current_tcb(void);
void __tcc_cpp_tls_n6_tcb_inspect(void *tcb, DWORD *owner_tid, DWORD *cleanup_tid,
                                  unsigned *hook_count, int *cleanup_state,
                                  unsigned *dtor_count);
}

static int descA, descB, descC, descT;
static CRITICAL_SECTION g_cs;

// N6-04: per-worker drain log.  The worker stores its slot (1 or 2) in
// ((int *)object)[1] after construction; the dtor callback reads it back so
// two concurrently exiting workers never share a log without a lock.
struct DrainLog {
    char order[8];
    DWORD tid[8];
    volatile int n;
};
static DrainLog g_drain[3];

static void log_dtor(void *p, char tag)
{
    int slot = ((int *)p)[1];
    DrainLog *d = &g_drain[(slot >= 1 && slot <= 2) ? slot : 0];
    if (d->n < 8) {
        d->order[d->n] = tag;
        d->tid[d->n] = GetCurrentThreadId();
    }
    ++d->n;
}

static void ctorA(void *p) { *(int *)p = 0xA; }
static void ctorB(void *p) { *(int *)p = 0xB; }
static unsigned g_registry_seen_in_ctorC[2];
static volatile int g_ctorC_slot;
static void ctorC(void *p)
{
    int slot;
    *(int *)p = 0xC;
    // Only A must be registered at this point; C itself is INITIALIZING.
    EnterCriticalSection(&g_cs);
    slot = g_ctorC_slot++;
    LeaveCriticalSection(&g_cs);
    if (slot < 2)
        g_registry_seen_in_ctorC[slot] = __tcc_cpp_tls_n6_registry_snapshot(0, 0, 0);
}
static void dtorA(void *p) { log_dtor(p, 'A'); }
static void dtorB(void *p) { log_dtor(p, 'B'); }
static void dtorC(void *p) { log_dtor(p, 'C'); }

struct Result {
    HANDLE go;
    int slot;
    volatile int ready;
    volatile DWORD tid;
    void *volatile tcb;
    void *a, *b, *c, *t, *a_again;
    unsigned count_after_3;
    unsigned count_after_a_again;
    void *objects[8];
    void *dtors[8];
    unsigned snapshot_count;
};

static DWORD WINAPI worker(void *p)
{
    Result *r = (Result *)p;
    r->tid = GetCurrentThreadId();
    r->a = __tcc_cpp_tls_addr(&descA, 16, ctorA, dtorA);
    ((int *)r->a)[1] = r->slot;
    r->c = __tcc_cpp_tls_addr(&descC, 16, ctorC, dtorC);
    ((int *)r->c)[1] = r->slot;
    r->t = __tcc_cpp_tls_addr(&descT, 16, 0, 0);
    r->b = __tcc_cpp_tls_addr(&descB, 16, ctorB, dtorB);
    ((int *)r->b)[1] = r->slot;
    r->count_after_3 = __tcc_cpp_tls_n6_registry_snapshot(0, 0, 0);
    r->a_again = __tcc_cpp_tls_addr(&descA, 16, ctorA, dtorA);
    r->count_after_a_again = __tcc_cpp_tls_n6_registry_snapshot(0, 0, 0);
    r->snapshot_count = __tcc_cpp_tls_n6_registry_snapshot(r->objects, r->dtors, 8);
    r->tcb = __tcc_cpp_tls_n6_current_tcb();
    r->ready = 1;
    WaitForSingleObject(r->go, INFINITE);
    return 0;
}

static int contains(void **arr, unsigned n, void *p)
{
    unsigned i;
    for (i = 0; i < n; ++i)
        if (arr[i] == p)
            return 1;
    return 0;
}

static int check_worker(const char *name, Result *r)
{
    int ok = 1;
    int order_ok = (r->snapshot_count == 3 && r->objects[0] == r->a
                    && r->objects[1] == r->c && r->objects[2] == r->b
                    && r->dtors[0] == (void *)dtorA && r->dtors[1] == (void *)dtorC
                    && r->dtors[2] == (void *)dtorB);
    int once_ok = (r->a_again == r->a && r->count_after_3 == 3
                   && r->count_after_a_again == 3);
    int trivial_ok = (r->t != 0 && *(int *)r->t == 0
                      && !contains(r->objects, r->snapshot_count, r->t));
    int values_ok = (*(int *)r->a == 0xA && *(int *)r->b == 0xB && *(int *)r->c == 0xC);
    printf("%s: registry=[%s,%s,%s] count=%u order=%s exactly_once=%s trivial_not_registered=%s values=%s\n",
           name,
           r->objects[0] == r->a ? "A" : "?", r->objects[1] == r->c ? "C" : "?",
           r->objects[2] == r->b ? "B" : "?", r->snapshot_count,
           order_ok ? "PASS" : "FAIL", once_ok ? "PASS" : "FAIL",
           trivial_ok ? "PASS" : "FAIL", values_ok ? "PASS" : "FAIL");
    ok = order_ok && once_ok && trivial_ok && values_ok;
    return ok;
}

int main()
{
    Result ra, rb;
    HANDLE ha, hb;
    int fail = 0;
    int a_in_b, b_in_a;
    DWORD cleanup_tid = 0;
    unsigned hooks = 0, dtors = 0;
    int state = 0;

    InitializeCriticalSection(&g_cs);
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    memset(g_drain, 0, sizeof g_drain);
    ra.slot = 1;
    rb.slot = 2;

    // main thread has no TCB yet: snapshot must report 0 without creating one.
    if (__tcc_cpp_tls_n6_registry_snapshot(0, 0, 0) != 0 || __tcc_cpp_tls_n6_current_tcb() != 0) {
        printf("MAIN_HAS_UNEXPECTED_TCB\n");
        fail = 1;
    }

    ra.go = rb.go = CreateEvent(NULL, TRUE, FALSE, NULL);
    ha = CreateThread(NULL, 0, worker, &ra, 0, NULL);
    hb = CreateThread(NULL, 0, worker, &rb, 0, NULL);
    while (!ra.ready || !rb.ready)
        Sleep(1);

    // both alive: registries are per-thread
    if (!check_worker("WORKER_A", &ra)) fail = 1;
    if (!check_worker("WORKER_B", &rb)) fail = 1;
    a_in_b = contains(rb.objects, rb.snapshot_count, ra.a)
             || contains(rb.objects, rb.snapshot_count, ra.b)
             || contains(rb.objects, rb.snapshot_count, ra.c);
    b_in_a = contains(ra.objects, ra.snapshot_count, rb.a)
             || contains(ra.objects, ra.snapshot_count, rb.b)
             || contains(ra.objects, ra.snapshot_count, rb.c);
    printf("A_REGISTRY_CONTAINS_B=%s\nB_REGISTRY_CONTAINS_A=%s\n",
           b_in_a ? "YES" : "NO", a_in_b ? "YES" : "NO");
    if (a_in_b || b_in_a || ra.tcb == rb.tcb || ra.a == rb.a)
        fail = 1;
    printf("NO_REGISTRATION_WHILE_INITIALIZING=%s (ctorC saw %u and %u registered, expected 1 and 1)\n",
           (g_registry_seen_in_ctorC[0] == 1 && g_registry_seen_in_ctorC[1] == 1) ? "PASS" : "FAIL",
           g_registry_seen_in_ctorC[0], g_registry_seen_in_ctorC[1]);
    if (!(g_registry_seen_in_ctorC[0] == 1 && g_registry_seen_in_ctorC[1] == 1))
        fail = 1;
    // while both are alive nothing has been drained
    printf("DTOR_CALLBACKS_WHILE_ALIVE=%d,%d (expected 0,0)\n", g_drain[1].n, g_drain[2].n);
    if (g_drain[1].n != 0 || g_drain[2].n != 0)
        fail = 1;

    SetEvent(ra.go);
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha);
    CloseHandle(hb);
    CloseHandle(ra.go);

    // After join (N6-04): each worker's TCB was hooked once on its own thread,
    // the drain finished (cleanup_state=2) and the registry is empty.  Reading
    // the TCB after join is valid only because N6-04A never frees it.
    __tcc_cpp_tls_n6_tcb_inspect(ra.tcb, 0, &cleanup_tid, &hooks, &state, &dtors);
    if (hooks != 1 || dtors != 0 || state != 2 || cleanup_tid != ra.tid) {
        printf("WORKER_A_TCB_AFTER_JOIN=FAIL hooks=%u state=%d dtors=%u cleanup_tid_eq=%d\n", hooks, state, dtors, cleanup_tid == ra.tid);
        fail = 1;
    } else {
        printf("WORKER_A_TCB_AFTER_JOIN=PASS hook_count=1 cleanup_state=2 dtor_count=0 cleanup_tid=A\n");
    }
    __tcc_cpp_tls_n6_tcb_inspect(rb.tcb, 0, &cleanup_tid, &hooks, &state, &dtors);
    if (hooks != 1 || dtors != 0 || state != 2 || cleanup_tid != rb.tid) {
        printf("WORKER_B_TCB_AFTER_JOIN=FAIL hooks=%u state=%d dtors=%u cleanup_tid_eq=%d\n", hooks, state, dtors, cleanup_tid == rb.tid);
        fail = 1;
    } else {
        printf("WORKER_B_TCB_AFTER_JOIN=PASS hook_count=1 cleanup_state=2 dtor_count=0 cleanup_tid=B\n");
    }
    // N6-04 drain: registered [A,C,B] -> drained B,C,A, once each, on the owner.
    {
        int i;
        int a_ok, b_ok;
        g_drain[1].order[3] = 0;
        g_drain[2].order[3] = 0;
        a_ok = g_drain[1].n == 3 && strcmp(g_drain[1].order, "BCA") == 0;
        b_ok = g_drain[2].n == 3 && strcmp(g_drain[2].order, "BCA") == 0;
        for (i = 0; i < 3; ++i) {
            if (g_drain[1].tid[i] != ra.tid) a_ok = 0;
            if (g_drain[2].tid[i] != rb.tid) b_ok = 0;
        }
        printf("WORKER_A_DRAIN: n=%d order=%s tids_eq_owner=%s -> %s\n", g_drain[1].n, g_drain[1].order,
               (g_drain[1].tid[0] == ra.tid && g_drain[1].tid[1] == ra.tid && g_drain[1].tid[2] == ra.tid) ? "YES" : "NO",
               a_ok ? "PASS" : "FAIL");
        printf("WORKER_B_DRAIN: n=%d order=%s tids_eq_owner=%s -> %s\n", g_drain[2].n, g_drain[2].order,
               (g_drain[2].tid[0] == rb.tid && g_drain[2].tid[1] == rb.tid && g_drain[2].tid[2] == rb.tid) ? "YES" : "NO",
               b_ok ? "PASS" : "FAIL");
        printf("UNATTRIBUTED_DTOR_CALLBACKS=%d (expected 0)\n", g_drain[0].n);
        if (!a_ok || !b_ok || g_drain[0].n != 0)
            fail = 1;
    }

    if (fail) {
        printf("N6_03_DTOR_REGISTRY=FAIL\n");
        return 1;
    }
    printf("N6_03_DTOR_REGISTRY=PASS\n");
    printf("N6_03_PER_THREAD_REGISTRY_ISOLATION=PASS\n");
    printf("N6_03_REGISTRATION_EXACTLY_ONCE=PASS\n");
    printf("N6_03_REGISTRATION_CONSTRUCTION_ORDER=ACTUAL_CONSTRUCTION_ORDER\n");
    printf("N6_04_RAW_ABI_DRAIN=LIFO_PASS\n");
    printf("DTOR_REGISTRY_OWNER=PER_THREAD_TCB\n");
    printf("N6_USES_N5_DTOR_REGISTRY=NO\n");
    return 0;
}
