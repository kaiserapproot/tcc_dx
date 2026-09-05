// N6-03 Authority: per-thread destructor registry foundation.
//
// The frontend still rejects thread_local classes with non-trivial
// destructors (NONTRIVIAL_DTOR_FRONTEND_ACCEPTANCE=NO), so generated code
// always passes dtor=NULL.  The registry is therefore exercised directly
// through the runtime ABI `__tcc_cpp_tls_addr(desc, size, ctor, dtor)` from
// two concurrent workers, and read back with the N6 inspection API.
//
// Measured:
//   N6_03_REGISTRATION_CONSTRUCTION_ORDER  A, C, B constructed -> [A, C, B]
//   N6_03_REGISTRATION_EXACTLY_ONCE        second access to A adds nothing
//   N6_03_PER_THREAD_REGISTRY_ISOLATION    worker A's objects never appear in
//                                          worker B's registry and vice versa
//   NO_REGISTRATION_WHILE_INITIALIZING     C's ctor sees only [A] registered
//   TRIVIAL_OBJECT_NOT_REGISTERED          dtor=NULL object (T) is absent
//   DTOR_EXECUTION=NOT_IMPLEMENTED         no dtor callback ever runs, even
//                                          after both workers have exited
//   exit records carry dtor_count=3 for each worker (hook saw the registry)
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_addr(int *descriptor, unsigned size, void (*ctor)(void *),
                         void (*dtor)(void *));
unsigned __tcc_cpp_tls_n6_registry_snapshot(void **objects, void **dtors,
                                            unsigned max);
void *__tcc_cpp_tls_n6_current_tcb(void);
int __tcc_cpp_tls_n6_exit_record(DWORD tid, void **tcb, unsigned *hook_count,
                                 unsigned *dtor_count);
}

static int descA, descB, descC, descT;
static volatile long g_dtor_calls;
static CRITICAL_SECTION g_cs;

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
static void dtorA(void *p) { EnterCriticalSection(&g_cs); ++g_dtor_calls; LeaveCriticalSection(&g_cs); }
static void dtorB(void *p) { EnterCriticalSection(&g_cs); ++g_dtor_calls; LeaveCriticalSection(&g_cs); }
static void dtorC(void *p) { EnterCriticalSection(&g_cs); ++g_dtor_calls; LeaveCriticalSection(&g_cs); }

struct Result {
    HANDLE go;
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
    r->c = __tcc_cpp_tls_addr(&descC, 16, ctorC, dtorC);
    r->t = __tcc_cpp_tls_addr(&descT, 16, 0, 0);
    r->b = __tcc_cpp_tls_addr(&descB, 16, ctorB, dtorB);
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
    void *tcb = 0;
    unsigned hooks = 0, dtors = 0;

    InitializeCriticalSection(&g_cs);
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);

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

    SetEvent(ra.go);
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha);
    CloseHandle(hb);
    CloseHandle(ra.go);

    // hook saw each registry (dtor_count=3) but ran nothing.
    if (__tcc_cpp_tls_n6_exit_record(ra.tid, &tcb, &hooks, &dtors) != 1 || hooks != 1 || dtors != 3 || tcb != ra.tcb) {
        printf("WORKER_A_EXIT_RECORD=FAIL hooks=%u dtors=%u\n", hooks, dtors);
        fail = 1;
    } else {
        printf("WORKER_A_EXIT_RECORD=PASS hook_count=1 dtor_count=3\n");
    }
    if (__tcc_cpp_tls_n6_exit_record(rb.tid, &tcb, &hooks, &dtors) != 1 || hooks != 1 || dtors != 3 || tcb != rb.tcb) {
        printf("WORKER_B_EXIT_RECORD=FAIL hooks=%u dtors=%u\n", hooks, dtors);
        fail = 1;
    } else {
        printf("WORKER_B_EXIT_RECORD=PASS hook_count=1 dtor_count=3\n");
    }
    printf("DTOR_CALLBACKS_EXECUTED=%ld (expected 0: N6_03_DTOR_EXECUTION=NOT_IMPLEMENTED)\n", g_dtor_calls);
    if (g_dtor_calls != 0)
        fail = 1;

    if (fail) {
        printf("N6_03_DTOR_REGISTRY=FAIL\n");
        return 1;
    }
    printf("N6_03_DTOR_REGISTRY=PASS\n");
    printf("N6_03_PER_THREAD_REGISTRY_ISOLATION=PASS\n");
    printf("N6_03_REGISTRATION_EXACTLY_ONCE=PASS\n");
    printf("N6_03_REGISTRATION_CONSTRUCTION_ORDER=ACTUAL_CONSTRUCTION_ORDER\n");
    printf("N6_03_DRAIN=NOT_IMPLEMENTED\n");
    printf("N6_03_DTOR_EXECUTION=NOT_IMPLEMENTED\n");
    printf("DTOR_REGISTRY_OWNER=PER_THREAD_TCB\n");
    printf("N6_USES_N5_DTOR_REGISTRY=NO\n");
    return 0;
}
