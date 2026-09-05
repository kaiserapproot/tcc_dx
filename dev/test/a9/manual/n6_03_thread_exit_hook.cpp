// N6-03 Authority: thread-exit hook in a tcc-built normal Windows EXE.
//
// Mechanism under test: PE TLS callback (IMAGE_TLS_DIRECTORY published by
// tccpe.c, callback list in the injected TLS runtime).  The loader calls the
// runtime hook with DLL_THREAD_DETACH on the exiting thread.  The hook records
// (tid, tcb, hook_count, dtor_count) into a process-global log that this test
// reads back through the N6 inspection API after joining each worker.
//
// Measured:
//   N6_03_THREAD_RETURN_HOOK     thread proc returns normally
//   N6_03_EXITTHREAD_HOOK        thread proc calls ExitThread()
//   N6_03_TWO_CONCURRENT_THREADS A and B alive at the same time
//   N6_03_THREAD_RECREATE        3 sequential threads after the above
//   HOOK_EXACTLY_ONCE_PER_THREAD hook_count == 1 for every worker
//   HOOK_RUNS_ON_EXITING_THREAD  record tid (GetCurrentThreadId() inside the
//                                hook) == worker tid
//   HOOK_HAS_CURRENT_TCB         record tcb == TCB the worker saw while alive
//   NO_TLS_THREAD_NO_HOOK_RECORD a worker that never touches TLS leaves no
//                                record (no TCB -> nothing to clean up)
// main return / exit / ExitProcess are N6-05 and are not measured here;
// TerminateThread is out of scope.
#include <windows.h>
#include <stdio.h>

extern "C" {
void *__tcc_cpp_tls_n6_current_tcb(void);
int __tcc_cpp_tls_n6_exit_record(DWORD tid, void **tcb, unsigned *hook_count,
                                 unsigned *dtor_count);
unsigned __tcc_cpp_tls_n6_exit_record_total(void);
}

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
};

static DWORD WINAPI worker(void *p)
{
    WorkerArg *a = (WorkerArg *)p;
    a->tid = GetCurrentThreadId();
    if (a->touch_tls) {
        a->value_seen = object.value;
        a->tcb = __tcc_cpp_tls_n6_current_tcb();
    } else {
        a->tcb = 0;
    }
    a->ready = 1;
    if (a->go)
        WaitForSingleObject(a->go, INFINITE);
    if (a->use_exit_thread)
        ExitThread(7);
    return 7;
}

static int g_fail;

static void check_record(const char *name, WorkerArg *a)
{
    void *tcb = 0;
    unsigned hooks = 0, dtors = 0;
    int found = __tcc_cpp_tls_n6_exit_record(a->tid, &tcb, &hooks, &dtors);
    int ok = (found == 1 && hooks == 1 && tcb == a->tcb && a->tcb != 0
              && a->value_seen == 123 && dtors == 0);
    printf("%s: records_for_tid=%d hook_count=%u hook_tid_eq_worker_tid=%s "
           "hook_tcb_eq_worker_tcb=%s dtor_count=%u value=%d -> %s\n",
           name, found, hooks, found ? "YES" : "NO",
           (found && tcb == a->tcb) ? "YES" : "NO", dtors, a->value_seen,
           ok ? "PASS" : "FAIL");
    if (!ok)
        g_fail = 1;
}

static void run_one(const char *name, int use_exit_thread)
{
    WorkerArg a;
    HANDLE th;
    DWORD rc = 0;
    memset(&a, 0, sizeof a);
    a.touch_tls = 1;
    a.use_exit_thread = use_exit_thread;
    th = CreateThread(NULL, 0, worker, &a, 0, NULL);
    if (!th) { printf("%s: CreateThread failed\n", name); g_fail = 1; return; }
    WaitForSingleObject(th, INFINITE);
    GetExitCodeThread(th, &rc);
    CloseHandle(th);
    if (rc != 7) { printf("%s: exit code %lu\n", name, rc); g_fail = 1; }
    check_record(name, &a);
}

int main()
{
    WorkerArg a, b, none;
    HANDLE ha, hb, hn;
    unsigned total_before, total_after;
    void *tcb_main = 0;
    unsigned hooks_main = 0;
    int i;

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
    ha = CreateThread(NULL, 0, worker, &a, 0, NULL);
    hb = CreateThread(NULL, 0, worker, &b, 0, NULL);
    while (!a.ready || !b.ready)
        Sleep(1);
    if (a.tcb == b.tcb) { printf("TWO_CONCURRENT: TCBs not isolated\n"); g_fail = 1; }
    SetEvent(a.go);
    WaitForSingleObject(ha, INFINITE);
    WaitForSingleObject(hb, INFINITE);
    CloseHandle(ha);
    CloseHandle(hb);
    CloseHandle(a.go);
    check_record("N6_03_TWO_CONCURRENT_THREADS_A", &a);
    check_record("N6_03_TWO_CONCURRENT_THREADS_B", &b);

    // 4. recreate: three more sequential threads, alternating exit style
    for (i = 0; i < 3; ++i) {
        char nm[48];
        sprintf(nm, "N6_03_THREAD_RECREATE_%d", i);
        run_one(nm, i & 1);
    }

    // 5. a thread that never touches TLS must not produce a record
    total_before = __tcc_cpp_tls_n6_exit_record_total();
    memset(&none, 0, sizeof none);
    none.touch_tls = 0;
    hn = CreateThread(NULL, 0, worker, &none, 0, NULL);
    WaitForSingleObject(hn, INFINITE);
    CloseHandle(hn);
    total_after = __tcc_cpp_tls_n6_exit_record_total();
    printf("NO_TLS_THREAD_NO_HOOK_RECORD=%s (records %u -> %u, found=%d)\n",
           (total_after == total_before
            && __tcc_cpp_tls_n6_exit_record(none.tid, 0, 0, 0) == 0) ? "PASS" : "FAIL",
           total_before, total_after, __tcc_cpp_tls_n6_exit_record(none.tid, 0, 0, 0));
    if (total_after != total_before)
        g_fail = 1;

    // 6. the main thread (still running) has no detach record
    if (__tcc_cpp_tls_n6_exit_record(GetCurrentThreadId(), &tcb_main, &hooks_main, 0) != 0) {
        printf("MAIN_THREAD_HOOK_BEFORE_EXIT=YES (unexpected)\n");
        g_fail = 1;
    } else {
        printf("MAIN_THREAD_HOOK_BEFORE_EXIT=NO\n");
    }
    printf("TOTAL_EXIT_RECORDS=%u (expected 7)\n", total_after);
    if (total_after != 7)
        g_fail = 1;

    if (g_fail) {
        printf("N6_03_THREAD_EXIT_HOOK=FAIL\n");
        return 1;
    }
    printf("N6_03_THREAD_EXIT_HOOK=PASS\n");
    printf("THREAD_EXIT_HOOK_MECHANISM=PE_TLS_CALLBACK_DLL_THREAD_DETACH\n");
    printf("HOOK_EXACTLY_ONCE_PER_THREAD=PASS\n");
    printf("HOOK_RUNS_ON_EXITING_THREAD=PASS\n");
    printf("HOOK_HAS_CURRENT_TCB=PASS\n");
    printf("MAIN_THREAD_EXIT_HOOK=DEFERRED_TO_N6_05\n");
    printf("FORCED_THREAD_TERMINATION=OUT_OF_SCOPE\n");
    return 0;
}
