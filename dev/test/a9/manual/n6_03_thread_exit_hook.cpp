// N6-03 Authority: thread-exit hook in a tcc-built normal Windows EXE.
//
// Mechanism under test: PE TLS callback (IMAGE_TLS_DIRECTORY published by
// tccpe.c, callback list in the injected TLS runtime).  The loader calls the
// runtime hook with DLL_THREAD_DETACH on the exiting thread.  The hook only
// marks the exiting thread's own TCB (hook_count / cleanup_tid /
// cleanup_state): no process-global log, no lock, no malloc/free, no user
// destructor (N6-03 review hardening).
//
// Measurement method: each worker captures its TCB pointer with
// __tcc_cpp_tls_n6_current_tcb() while alive.  The hook itself is observed
// from INSIDE it: the worker also touches `thread_local Probe probe`, whose
// destructor runs in the hook's drain (N6-04) on the exiting thread while the
// TCB is still alive, and records __tcc_cpp_tls_n6_tcb_inspect() of the
// current TCB into the WorkerArg (hook_count, cleanup_state=1 DRAINING,
// cleanup_tid, owner_tid) plus GetCurrentThreadId().  After the join the TCB
// has been reclaimed (N6-04B), so the main thread never dereferences it; it
// checks the process-wide accounting counters instead (hook delivered, drain
// completed, TCB freed: +1 per worker).  P itself has no destructor: the
// hook is delivered whether or not the registry is empty.
//
// Measured:
//   N6_03_THREAD_RETURN_HOOK     thread proc returns normally
//   N6_03_EXITTHREAD_HOOK        thread proc calls ExitThread()
//   N6_03_TWO_CONCURRENT_THREADS A and B alive at the same time
//   N6_03_THREAD_RECREATE        3 sequential threads after the above
//   HOOK_EXACTLY_ONCE_PER_THREAD hook_count == 1 seen from the hook, probe
//                                dtor ran once, HOOK_DELIVERED +1 per worker
//   HOOK_RUNS_ON_EXITING_THREAD  cleanup_tid (GetCurrentThreadId() inside
//                                the hook) == worker tid == owner_tid ==
//                                the tid the probe dtor observed
//   HOOK_HAS_CURRENT_TCB         the TCB seen inside the hook is exactly the
//                                TCB the worker captured while alive
//   NO_TLS_THREAD_NO_TCB         a worker that never touches TLS has no TCB
// main return / exit / ExitProcess are N6-05 and are not measured here;
// TerminateThread is out of scope.
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
       ST_CROSS_THREAD_RECLAIM_SKIPPED, ST_DTOR_CALLS, ST_COUNT };

struct P {
    int value;
    P() { value = 123; }
};

thread_local P object;

struct WorkerArg {
    int touch_tls;
    int use_exit_thread;
    HANDLE go;
    volatile DWORD tid;
    void *volatile tcb;
    volatile int ready;
    volatile int value_seen;
    volatile unsigned hook_count_while_alive;
    volatile int cleanup_state_while_alive;
    // written by Probe::~Probe inside the hook (thread still alive)
    volatile int probe_dtor_calls;
    volatile DWORD probe_dtor_tid;
    void *volatile tcb_in_hook;
    volatile DWORD owner_in_hook;
    volatile DWORD cleanup_tid_in_hook;
    volatile unsigned hook_count_in_hook;
    volatile int cleanup_state_in_hook;
    volatile unsigned dtor_count_in_hook;
};

// Observer object: its destructor is the only code that runs inside the hook.
struct Probe {
    WorkerArg *arg;
    Probe() { arg = 0; }
    ~Probe();
};
thread_local Probe probe;

Probe::~Probe()
{
    WorkerArg *a = arg;
    DWORD owner = 0, cleanup_tid = 0;
    unsigned hooks = 0, dtors = 9;
    int state = 0;
    if (!a)
        return;
    ++a->probe_dtor_calls;
    a->probe_dtor_tid = GetCurrentThreadId();
    a->tcb_in_hook = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_tcb_inspect(a->tcb_in_hook, &owner, &cleanup_tid, &hooks, &state, &dtors);
    a->owner_in_hook = owner;
    a->cleanup_tid_in_hook = cleanup_tid;
    a->hook_count_in_hook = hooks;
    a->cleanup_state_in_hook = state;
    a->dtor_count_in_hook = dtors;
}

static DWORD WINAPI worker(void *p)
{
    WorkerArg *a = (WorkerArg *)p;
    unsigned hc = 0;
    int cs = 0;
    a->tid = GetCurrentThreadId();
    if (a->touch_tls) {
        a->value_seen = object.value;
        probe.arg = a;
        a->tcb = __tcc_cpp_tls_n6_current_tcb();
        __tcc_cpp_tls_n6_tcb_inspect(a->tcb, 0, 0, &hc, &cs, 0);
        a->hook_count_while_alive = hc;
        a->cleanup_state_while_alive = cs;
    } else {
        a->tcb = __tcc_cpp_tls_n6_current_tcb();
    }
    a->ready = 1;
    if (a->go)
        WaitForSingleObject(a->go, INFINITE);
    if (a->use_exit_thread)
        ExitThread(7);
    return 7;
}

static int g_fail;

// Check what the hook recorded on the worker (after the thread has been
// joined; the TCB itself is not touched, it was reclaimed by N6-04B).
static void check_tcb(const char *name, WorkerArg *a)
{
    int ok;
    ok = (a->tcb != 0 && a->value_seen == 123
          && a->hook_count_while_alive == 0 && a->cleanup_state_while_alive == 0
          && a->probe_dtor_calls == 1 && a->probe_dtor_tid == a->tid
          && a->tcb_in_hook == a->tcb
          && a->hook_count_in_hook == 1 && a->cleanup_state_in_hook == 1
          && a->owner_in_hook == a->tid && a->cleanup_tid_in_hook == a->tid
          && a->dtor_count_in_hook == 0);
    printf("%s: tcb=%s alive{hook_count=%u state=%d} in_hook{same_tcb=%s hook_count=%u state=%d "
           "owner_tid_eq_worker=%s cleanup_tid_eq_worker=%s dtor_count=%u probe_dtor=%d on_worker=%s} value=%d -> %s\n",
           name, a->tcb ? "captured" : "NULL",
           a->hook_count_while_alive, a->cleanup_state_while_alive,
           a->tcb_in_hook == a->tcb ? "YES" : "NO",
           a->hook_count_in_hook, a->cleanup_state_in_hook,
           a->owner_in_hook == a->tid ? "YES" : "NO",
           a->cleanup_tid_in_hook == a->tid ? "YES" : "NO",
           a->dtor_count_in_hook, a->probe_dtor_calls,
           a->probe_dtor_tid == a->tid ? "YES" : "NO",
           a->value_seen, ok ? "PASS" : "FAIL");
    if (!ok)
        g_fail = 1;
}

// Process-wide accounting delta: `n` workers with TLS exited since `st0`.
static void check_stats(const char *name, long *st0, long n)
{
    long st1[ST_COUNT];
    long d_hook, d_drain, d_tcb_f;
    __tcc_cpp_tls_n6_stats(st1, ST_COUNT);
    d_hook = st1[ST_HOOK_DELIVERED] - st0[ST_HOOK_DELIVERED];
    d_drain = st1[ST_DRAIN_COMPLETED] - st0[ST_DRAIN_COMPLETED];
    d_tcb_f = st1[ST_TCB_FREE] - st0[ST_TCB_FREE];
    printf("%s_ACCOUNTING: hooks=%ld drains=%ld tcb_free=%ld (expected %ld each) -> %s\n",
           name, d_hook, d_drain, d_tcb_f, n,
           (d_hook == n && d_drain == n && d_tcb_f == n) ? "PASS" : "FAIL");
    if (!(d_hook == n && d_drain == n && d_tcb_f == n))
        g_fail = 1;
}

static void run_one(const char *name, int use_exit_thread)
{
    WorkerArg a;
    HANDLE th;
    DWORD rc = 0;
    long st0[ST_COUNT];
    memset(&a, 0, sizeof a);
    a.touch_tls = 1;
    a.use_exit_thread = use_exit_thread;
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    th = CreateThread(NULL, 0, worker, &a, 0, NULL);
    if (!th) { printf("%s: CreateThread failed\n", name); g_fail = 1; return; }
    WaitForSingleObject(th, INFINITE);
    GetExitCodeThread(th, &rc);
    CloseHandle(th);
    if (rc != 7) { printf("%s: exit code %lu\n", name, rc); g_fail = 1; }
    check_tcb(name, &a);
    check_stats(name, st0, 1);
}

int main()
{
    WorkerArg a, b, none;
    HANDLE ha, hb, hn;
    void *tcb_main;
    unsigned hooks_main = 0;
    int state_main = 0;
    int i;
    long st0[ST_COUNT];

    // 1. normal thread-proc return
    run_one("N6_03_THREAD_RETURN_HOOK", 0);
    // 2. ExitThread()
    run_one("N6_03_EXITTHREAD_HOOK", 1);

    // 3. two concurrent workers, both alive with constructed TLS at once
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.touch_tls = b.touch_tls = 1;
    a.use_exit_thread = 0;
    b.use_exit_thread = 1;
    a.go = b.go = CreateEvent(NULL, TRUE, FALSE, NULL);
    __tcc_cpp_tls_n6_stats(st0, ST_COUNT);
    ha = CreateThread(NULL, 0, worker, &a, 0, NULL);
    hb = CreateThread(NULL, 0, worker, &b, 0, NULL);
    while (!a.ready || !b.ready)
        Sleep(1);
    if (a.tcb == b.tcb) { printf("TWO_CONCURRENT: TCBs not isolated\n"); g_fail = 1; }
    // while both are alive neither TCB has been hooked
    {
        unsigned ha_c = 9, hb_c = 9;
        __tcc_cpp_tls_n6_tcb_inspect(a.tcb, 0, 0, &ha_c, 0, 0);
        __tcc_cpp_tls_n6_tcb_inspect(b.tcb, 0, 0, &hb_c, 0, 0);
        printf("TWO_CONCURRENT_BEFORE_EXIT: A_HOOK_COUNT=%u B_HOOK_COUNT=%u\n", ha_c, hb_c);
        if (ha_c != 0 || hb_c != 0) g_fail = 1;
    }
    SetEvent(a.go);
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha);
    CloseHandle(hb);
    CloseHandle(a.go);
    check_tcb("N6_03_TWO_CONCURRENT_THREADS_A", &a);
    check_tcb("N6_03_TWO_CONCURRENT_THREADS_B", &b);
    check_stats("N6_03_TWO_CONCURRENT_THREADS", st0, 2);
    printf("A_HOOK_COUNT=%u\nB_HOOK_COUNT=%u\nA_HOOK_THREAD_ID=%s\nB_HOOK_THREAD_ID=%s\n",
           a.hook_count_in_hook, b.hook_count_in_hook,
           a.cleanup_tid_in_hook == a.tid ? "A_THREAD_ID" : "MISMATCH",
           b.cleanup_tid_in_hook == b.tid ? "B_THREAD_ID" : "MISMATCH");

    // 4. recreate: three more sequential threads, alternating exit style
    for (i = 0; i < 3; ++i) {
        char nm[48];
        sprintf(nm, "N6_03_THREAD_RECREATE_%d", i);
        run_one(nm, i & 1);
    }

    // 5. a thread that never touches TLS gets no TCB (nothing for the hook)
    memset(&none, 0, sizeof none);
    none.touch_tls = 0;
    none.tcb = (void *)1;
    hn = CreateThread(NULL, 0, worker, &none, 0, NULL);
    WaitForSingleObject(hn, INFINITE);
    CloseHandle(hn);
    printf("NO_TLS_THREAD_NO_TCB=%s\n", none.tcb == 0 ? "PASS" : "FAIL");
    if (none.tcb != 0)
        g_fail = 1;

    // 6. the main thread (still running): its TCB, if any, is unhooked
    tcb_main = __tcc_cpp_tls_n6_current_tcb();
    __tcc_cpp_tls_n6_tcb_inspect(tcb_main, 0, 0, &hooks_main, &state_main, 0);
    printf("MAIN_THREAD_HOOK_BEFORE_EXIT=%s (tcb=%s hook_count=%u state=%d)\n",
           (hooks_main == 0 && state_main == 0) ? "NO" : "YES",
           tcb_main ? "present" : "none", hooks_main, state_main);
    if (hooks_main != 0 || state_main != 0)
        g_fail = 1;

    if (g_fail) {
        printf("N6_03_THREAD_EXIT_HOOK=FAIL\n");
        return 1;
    }
    printf("N6_03_THREAD_EXIT_HOOK=PASS\n");
    printf("THREAD_EXIT_HOOK_MECHANISM=PE_TLS_CALLBACK_DLL_THREAD_DETACH\n");
    printf("HOOK_STATE_OWNER=PER_THREAD_TCB\n");
    printf("HOOK_EXACTLY_ONCE_PER_THREAD=PASS\n");
    printf("HOOK_RUNS_ON_EXITING_THREAD=PASS\n");
    printf("HOOK_HAS_CURRENT_TCB=PASS\n");
    printf("MAIN_THREAD_EXIT_HOOK=DEFERRED_TO_N6_05\n");
    printf("FORCED_THREAD_TERMINATION=OUT_OF_SCOPE\n");
    return 0;
}
