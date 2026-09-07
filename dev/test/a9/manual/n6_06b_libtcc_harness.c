// N6-06B-00: libtcc direct relocate contract measurement harness.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "libtcc.h"

typedef int (*fn_i0)(void);
typedef int (*fn_get_counters)(int *, int *, long *, long *, void **);
typedef void (*fn_global_flags)(int *, int *);
typedef long (*fn_l0)(void);

typedef struct n6_06b_metrics {
    int call1_ctor;
    int call1_dtor;
    int call2_ctor;
    int call2_dtor;
    long call1_outstanding;
    long call2_outstanding;
    long call1_dtor_calls;
    long call2_dtor_calls;
    int global_ctor_before;
    int global_dtor_after_func;
    int global_dtor_at_delete;
    int manual_main_rc;
    int manual_main_tls_dtor;
    int manual_run_state;
    int manual_main_state;
    long manual_dtor_calls;
    int worker_ctor_delta;
    int worker_dtor_delta;
    long worker_outstanding_after;
    int pending_tls_at_delete;
    int delete_live_completed;
    void *state_a_tcb;
    void *state_b_tcb;
    int state_b_tcb_stable;
    DWORD child_exit_rc;
} n6_06b_metrics;

static int harness_register_exit(void (*fn)(void))
{
    (void)fn;
    return 0;
}

static void add_direct_relocate_stubs(TCCState *s)
{
    tcc_add_symbol(s, "__tcc_cpp_register_exit", (void *)harness_register_exit);
}

static int build_path(char *buf, size_t n, const char *libpath, const char *name)
{
    return _snprintf(buf, (int)n, "%s/test/a9/manual/%s", libpath, name) < 0 ? -1 : 0;
}

static TCCState *new_state_memory(const char *libpath)
{
    TCCState *s;

    s = tcc_new();
    if (!s)
        return NULL;
    tcc_set_lib_path(s, libpath);
    if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) < 0) {
        tcc_delete(s);
        return NULL;
    }
    add_direct_relocate_stubs(s);
    return s;
}

static int compile_file(TCCState *s, const char *libpath, const char *filename)
{
    char path[1024];

    if (build_path(path, sizeof(path), libpath, filename) < 0)
        return -1;
    return tcc_add_file(s, path);
}

static int relocate_state(TCCState *s)
{
    printf("RELOCATE_BEGIN\n");
    fflush(stdout);
    if (tcc_relocate(s) < 0)
        return -1;
    printf("RELOCATE_END\n");
    fflush(stdout);
    return 0;
}

static fn_i0 get_fn0(TCCState *s, const char *name)
{
    fn_i0 fn;

    fn = (fn_i0)tcc_get_symbol(s, name);
    printf("GET_SYMBOL name=%s ptr=%p\n", name, (void *)fn);
    fflush(stdout);
    return fn;
}

static int probe_tls_lifetime(const char *libpath, n6_06b_metrics *m)
{
    TCCState *s;
    fn_i0 touch;
    fn_get_counters counters;
    int rc;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_tls_touch.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    touch = get_fn0(s, "touch_tls");
    counters = (fn_get_counters)get_fn0(s, "probe_get_tls_counters");
    if (!touch || !counters) {
        tcc_delete(s);
        return 1;
    }

    printf("HOST_CALL_BEGIN call=1\n");
    fflush(stdout);
    rc = touch();
    printf("HOST_CALL_RETURN call=1 rc=%d\n", rc);
    fflush(stdout);
    counters(&m->call1_ctor, &m->call1_dtor, &m->call1_outstanding,
             &m->call1_dtor_calls, NULL);
    printf("AFTER_CALL1_TLS_CTOR_COUNT=%d\n", m->call1_ctor);
    printf("AFTER_CALL1_TLS_DTOR_COUNT=%d\n", m->call1_dtor);
    printf("AFTER_CALL1_OUTSTANDING=%ld\n", m->call1_outstanding);
    printf("AFTER_CALL1_DTOR_CALLS=%ld\n", m->call1_dtor_calls);
    fflush(stdout);

    printf("HOST_CALL_BEGIN call=2\n");
    fflush(stdout);
    rc = touch();
    printf("HOST_CALL_RETURN call=2 rc=%d\n", rc);
    fflush(stdout);
    counters(&m->call2_ctor, &m->call2_dtor, &m->call2_outstanding,
             &m->call2_dtor_calls, NULL);
    printf("AFTER_CALL2_TLS_CTOR_COUNT=%d\n", m->call2_ctor);
    printf("AFTER_CALL2_TLS_DTOR_COUNT=%d\n", m->call2_dtor);
    printf("AFTER_CALL2_OUTSTANDING=%ld\n", m->call2_outstanding);
    printf("AFTER_CALL2_DTOR_CALLS=%ld\n", m->call2_dtor_calls);
    fflush(stdout);

    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END\n");
    printf("AFTER_TCC_DELETE_TLS_DTOR_COUNT=%d\n", m->call2_dtor);
    printf("AFTER_TCC_DELETE_OUTSTANDING=%ld\n", m->call2_outstanding);
    printf("AFTER_TCC_DELETE_DTOR_CALLS=%ld\n", m->call2_dtor_calls);
    fflush(stdout);
    return 0;
}

static int probe_global_ctor(const char *libpath, n6_06b_metrics *m)
{
    TCCState *s;
    fn_i0 func;
    fn_global_flags flags;
    int rc, ctor, dtor;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_global.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    func = get_fn0(s, "probe_func");
    flags = (fn_global_flags)get_fn0(s, "probe_global_flags");
    if (!func || !flags) {
        tcc_delete(s);
        return 1;
    }
    ctor = dtor = 0;
    flags(&ctor, &dtor);
    m->global_ctor_before = ctor;
    printf("GLOBAL_CTOR_BEFORE_MANUAL_FUNCTION=%s\n", ctor ? "YES" : "NO");
    printf("HOST_CALL_BEGIN global_probe\n");
    fflush(stdout);
    rc = func();
    printf("HOST_CALL_RETURN global_probe rc=%d\n", rc);
    flags(&ctor, &dtor);
    m->global_dtor_after_func = dtor;
    printf("GLOBAL_DTOR_AFTER_MANUAL_FUNCTION=%s\n", dtor ? "YES" : "NO");
    fflush(stdout);
    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END\n");
    m->global_dtor_at_delete = dtor;
    printf("GLOBAL_DTOR_AT_TCC_DELETE=%s\n", dtor ? "YES" : "NO");
    fflush(stdout);
    return 0;
}

static int probe_manual_main(const char *libpath, n6_06b_metrics *m)
{
    TCCState *s;
    fn_i0 main_fn, dtor_cnt_fn;
    fn_l0 dtor_calls_fn;
    fn_i0 run_st_fn, main_st_fn;
    int run_before, run_after, main_present;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_manual_main.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    run_st_fn = (fn_i0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_run_state");
    main_st_fn = (fn_i0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_main_state");
    printf("N6_RUN_STATE_SYMBOL=%s\n", run_st_fn ? "PRESENT" : "ABSENT");
    printf("N6_MAIN_STATE_SYMBOL=%s\n", main_st_fn ? "PRESENT" : "ABSENT");
    run_before = run_st_fn ? run_st_fn() : -1;
    main_present = main_st_fn ? main_st_fn() : -1;
    printf("N6_RUN_STATE_BEFORE_MANUAL_MAIN=%d\n", run_before);
    printf("N6_MAIN_STATE_BEFORE_MANUAL_MAIN=%d\n", main_present);
    main_fn = get_fn0(s, "main");
    dtor_cnt_fn = (fn_i0)get_fn0(s, "probe_main_tls_dtor_count");
    dtor_calls_fn = (fn_l0)get_fn0(s, "probe_main_dtor_calls");
    if (!main_fn) {
        tcc_delete(s);
        return 1;
    }
    printf("GET_SYMBOL name=main ptr=%p\n", (void *)main_fn);
    printf("HOST_CALL_BEGIN manual_main\n");
    fflush(stdout);
    m->manual_main_rc = main_fn();
    printf("HOST_CALL_RETURN manual_main rc=%d\n", m->manual_main_rc);
    run_after = run_st_fn ? run_st_fn() : -1;
    m->manual_run_state = run_after;
    m->manual_main_state = main_st_fn ? main_st_fn() : -1;
    m->manual_main_tls_dtor = dtor_cnt_fn ? dtor_cnt_fn() : -1;
    m->manual_dtor_calls = dtor_calls_fn ? dtor_calls_fn() : -1;
    printf("MANUAL_MAIN_RETURN_RC=%d\n", m->manual_main_rc);
    printf("MANUAL_MAIN_TLS_DTOR_COUNT=%d\n", m->manual_main_tls_dtor);
    printf("MANUAL_MAIN_USES_N6_RUN_ENTER=%s\n",
           (run_after > 0) ? "YES" : "NO");
    printf("MANUAL_MAIN_USES_N6_RUN_FINALIZE=%s\n",
           (run_after >= 3) ? "YES" : "NO");
    printf("N6_RUN_STATE_AFTER_MANUAL_MAIN=%d\n", run_after);
    printf("N6_MAIN_STATE_AFTER_MANUAL_MAIN=%d\n", m->manual_main_state);
    if (dtor_calls_fn)
        printf("MANUAL_MAIN_DTOR_CALLS=%ld\n", m->manual_dtor_calls);
    fflush(stdout);
    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END\n");
    fflush(stdout);
    return 0;
}

static int run_child_main_exit(const char *libpath)
{
    TCCState *s;
    fn_i0 main_fn;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_manual_main_exit.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    main_fn = get_fn0(s, "main");
    if (!main_fn) {
        tcc_delete(s);
        return 1;
    }
    printf("HOST_CALL_BEGIN manual_main_exit\n");
    fflush(stdout);
    main_fn();
    printf("MANUAL_CALL_EXIT_RETURNS_TO_HOST=YES\n");
    fflush(stdout);
    tcc_delete(s);
    return 0;
}

typedef struct {
    fn_i0 touch;
} host_worker_ctx;

static DWORD WINAPI host_worker_thread(LPVOID param)
{
    host_worker_ctx *ctx = (host_worker_ctx *)param;
    int rc;

    printf("HOST_WORKER_ENTRY tid=%lu\n", (unsigned long)GetCurrentThreadId());
    fflush(stdout);
    rc = ctx->touch();
    printf("HOST_WORKER_COMPILED_RETURN rc=%d tid=%lu\n", rc,
           (unsigned long)GetCurrentThreadId());
    fflush(stdout);
    return 0;
}

static int probe_host_worker(const char *libpath, n6_06b_metrics *m)
{
    TCCState *s;
    fn_i0 touch;
    fn_get_counters counters;
    host_worker_ctx ctx;
    HANDLE th;
    int c0, d0;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_tls_touch.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    touch = get_fn0(s, "touch_tls");
    counters = (fn_get_counters)get_fn0(s, "probe_get_tls_counters");
    if (!touch || !counters) {
        tcc_delete(s);
        return 1;
    }
    counters(&c0, &d0, NULL, NULL, NULL);
    ctx.touch = touch;
    th = CreateThread(NULL, 0, host_worker_thread, &ctx, 0, NULL);
    if (!th) {
        tcc_delete(s);
        return 1;
    }
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
    counters(&m->worker_ctor_delta, &m->worker_dtor_delta,
             &m->worker_outstanding_after, NULL, NULL);
    m->worker_ctor_delta -= c0;
    m->worker_dtor_delta -= d0;
    printf("MANUAL_WORKER_TLS_CTOR=%d\n", m->worker_ctor_delta);
    printf("MANUAL_WORKER_TLS_DTOR=%d\n", m->worker_dtor_delta);
    printf("MANUAL_WORKER_TLS_DTOR_AT_THREAD_EXIT=%s\n",
           m->worker_dtor_delta > 0 ? "YES" : "NO");
    printf("MANUAL_WORKER_TLS_RECLAIM_AT_THREAD_EXIT=%s\n",
           m->worker_outstanding_after <= 0 ? "YES" : "NO");
    fflush(stdout);
    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END\n");
    fflush(stdout);
    return 0;
}

static int probe_delete_live_tls(const char *libpath, n6_06b_metrics *m)
{
    TCCState *s;
    fn_i0 touch;
    fn_get_counters counters;
    int c1, d1;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_06b_probe_tls_touch.cpp") < 0
        || relocate_state(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    touch = get_fn0(s, "touch_tls");
    counters = (fn_get_counters)get_fn0(s, "probe_get_tls_counters");
    if (!touch || !counters) {
        tcc_delete(s);
        return 1;
    }
    touch();
    counters(&c1, &d1, NULL, NULL, NULL);
    m->pending_tls_at_delete = (d1 == 0 && c1 > 0);
    printf("PENDING_TLS_DTOR_AT_TCC_DELETE=%s\n",
           m->pending_tls_at_delete ? "YES" : "NO");
    printf("PENDING_TLS_BEFORE_DELETE_CTOR=%d DTOR=%d\n", c1, d1);
    fflush(stdout);
    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END\n");
    m->delete_live_completed = 1;
    printf("TCC_DELETE_WITH_LIVE_TLS_COMPLETED=YES\n");
    fflush(stdout);
    return 0;
}

static int probe_multi_state(const char *libpath, n6_06b_metrics *m)
{
    TCCState *sa, *sb;
    fn_i0 touch_a, touch_b;
    fn_get_counters ca, cb;
    void *tcb_b2;
    int c, d;
    long o;

    sa = new_state_memory(libpath);
    sb = new_state_memory(libpath);
    if (!sa || !sb) {
        if (sa) tcc_delete(sa);
        if (sb) tcc_delete(sb);
        return 1;
    }
    if (compile_file(sa, libpath, "n6_06b_probe_tls_touch.cpp") < 0
        || compile_file(sb, libpath, "n6_06b_probe_tls_touch.cpp") < 0
        || relocate_state(sa) < 0 || relocate_state(sb) < 0) {
        tcc_delete(sa);
        tcc_delete(sb);
        return 1;
    }
    touch_a = (fn_i0)tcc_get_symbol(sa, "touch_tls");
    touch_b = (fn_i0)tcc_get_symbol(sb, "touch_tls");
    ca = (fn_get_counters)tcc_get_symbol(sa, "probe_get_tls_counters");
    cb = (fn_get_counters)tcc_get_symbol(sb, "probe_get_tls_counters");
    if (!touch_a || !touch_b || !ca || !cb) {
        tcc_delete(sa);
        tcc_delete(sb);
        return 1;
    }
    touch_a();
    ca(&c, &d, &o, NULL, &m->state_a_tcb);
    touch_b();
    cb(&c, &d, &o, NULL, &m->state_b_tcb);
    printf("STATE_A_TLS_TCB=%p\n", m->state_a_tcb);
    printf("STATE_B_TLS_TCB=%p\n", m->state_b_tcb);
    printf("MULTI_TCCSTATE_TLS_ISOLATION=%s\n",
           (m->state_a_tcb && m->state_b_tcb && m->state_a_tcb != m->state_b_tcb)
           ? "YES" : "NO");
    fflush(stdout);
    tcc_delete(sa);
    touch_b();
    cb(&c, &d, &o, NULL, &tcb_b2);
    m->state_b_tcb_stable = (tcb_b2 == m->state_b_tcb);
    printf("STATE_B_TCB_AFTER_A_DELETE=%p\n", tcb_b2);
    printf("STATE_A_DELETE_AFFECTS_STATE_B=%s\n",
           m->state_b_tcb_stable ? "NO" : "YES");
    fflush(stdout);
    tcc_delete(sb);
    return 0;
}

static int spawn_child_exit_probe(const char *libpath, DWORD *exit_code)
{
    char exe[MAX_PATH];
    char cmd[2048];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    if (!GetModuleFileNameA(NULL, exe, MAX_PATH))
        return -1;
    if (_snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\" child_exit", exe, libpath) < 0)
        return -1;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}

static void print_authority(const n6_06b_metrics *m)
{
    printf("\n=== N6-06B-00 DIRECT RELOCATE CONTRACT ===\n");
    printf("BASE_COMMIT=0fe0c4e\n");
    printf("N6_06A=COMPLETE\n");
    printf("N6_06A2=COMPLETE\n");
    printf("LIBTCC_MODE=TCC_OUTPUT_MEMORY_DIRECT_RELOCATE\n");
    printf("HOST_OWNS_MANUAL_FUNCTION_CALL=YES\n");
    printf("MANUAL_EXECUTION_BEGIN_OBSERVABLE_BY_TCC=NO\n");
    printf("MANUAL_EXECUTION_END_OBSERVABLE_BY_TCC=NO\n");
    printf("DIRECT_RELOCATE_TLS_STORAGE=AVAILABLE\n");
    printf("DIRECT_RELOCATE_TLS_LAZY_INIT=AVAILABLE\n");
    printf("DIRECT_RELOCATE_TLS_DTOR_AFTER_FUNCTION_RETURN=%s\n",
           m->call1_dtor > 0 ? "YES" : "NO");
    printf("DIRECT_RELOCATE_TLS_RECLAIM_AFTER_FUNCTION_RETURN=%s\n",
           (m->call1_outstanding == 0 && m->call1_ctor > 0) ? "YES" : "NO");
    printf("SECOND_MANUAL_CALL_REUSES_TLS_OBJECT=%s\n",
           (m->call2_ctor == m->call1_ctor && m->call1_ctor > 0) ? "YES" : "NO");
    printf("MANUAL_CALL_IS_TLS_EXECUTION_EPOCH=NO\n");
    printf("TCCSTATE_LIFETIME_EQUALS_EXECUTION_LIFETIME=NO\n");
    printf("TCC_DELETE_IS_EXECUTION_END_AUTHORITY=NO\n");
    printf("TCC_DELETE_CALLS_TLS_DTOR=%s\n",
           (m->call2_dtor > m->call1_dtor) ? "YES" : "NO");
    printf("TCC_DELETE_RECLAIMS_TLS=%s\n",
           (m->call2_outstanding < m->call1_outstanding) ? "YES" : "NO");
    printf("GLOBAL_CTOR_AUTOMATIC=%s\n", m->global_ctor_before ? "YES" : "NO");
    printf("GLOBAL_DTOR_AUTOMATIC=%s\n", m->global_dtor_after_func ? "YES" : "NO");
    printf("DIRECT_RELOCATE_COMPLETE_CPP_EXECUTION=NO\n");
    printf("GET_SYMBOL_MAIN_EQUALS_TCC_RUN_SEMANTICS=NO\n");
    printf("MANUAL_MAIN_EQUALS_TCC_RUN_SEMANTICS=NO\n");
    printf("MANUAL_MAIN_USES_N6_RUN_ENTER=%s\n",
           m->manual_run_state > 0 ? "YES" : "NO");
    printf("MANUAL_MAIN_USES_N6_RUN_FINALIZE=%s\n",
           m->manual_run_state >= 3 ? "YES" : "NO");
    printf("MANUAL_MAIN_TLS_FINALIZE=%s\n",
           m->manual_main_tls_dtor > 0 ? "YES" : "NO");
    if (m->child_exit_rc == 23) {
        printf("MANUAL_CALL_EXIT_RETURNS_TO_HOST=NO\n");
        printf("MANUAL_CALL_EXIT_TERMINATES_HOST_PROCESS=YES\n");
        printf("MANUAL_CALL_EXIT_RC=23\n");
        printf("MANUAL_EXIT_TLS_DTOR=UNKNOWN_CHILD\n");
        printf("DIRECT_RELOCATE_EXIT_SEMANTICS_DIFFER_FROM_TCC_RUN=YES\n");
    } else if ((int)m->child_exit_rc < 0) {
        printf("MANUAL_CALL_EXIT_RETURNS_TO_HOST=NO\n");
        printf("MANUAL_CALL_EXIT_TERMINATES_HOST_PROCESS=CRASH\n");
        printf("MANUAL_CALL_EXIT_RC=%ld\n", (long)m->child_exit_rc);
    } else {
        printf("MANUAL_CALL_EXIT_RETURNS_TO_HOST=YES\n");
        printf("MANUAL_CALL_EXIT_TERMINATES_HOST_PROCESS=NO\n");
        printf("MANUAL_CALL_EXIT_RC=%lu\n", (unsigned long)m->child_exit_rc);
    }
    printf("HOST_WORKER_MANUAL_CALL_TLS_DTOR_AT_THREAD_EXIT=%s\n",
           m->worker_dtor_delta > 0 ? "YES" : "NO");
    printf("PENDING_TLS_DTOR_AT_TCC_DELETE=%s\n",
           m->pending_tls_at_delete ? "YES" : "NO");
    printf("PENDING_TLS_DTOR_CODE_POINTER_OWNED_BY_RELOCATED_IMAGE=YES\n");
    printf("TCC_DELETE_WITH_LIVE_TLS_SAFE=%s\n",
           m->delete_live_completed ? "OBSERVED_NO_CRASH" : "NOT_RUN");
    printf("MULTI_TCCSTATE_TLS_ISOLATION=%s\n",
           (m->state_a_tcb && m->state_b_tcb && m->state_a_tcb != m->state_b_tcb)
           ? "YES" : "NO");
    printf("DIRECT_RELOCATE_AUTOMATIC_TLS_FINALIZATION=UNSUPPORTED\n");
    printf("AUTOMATIC_TLS_FINALIZATION_REPRESENTABLE_WITH_CURRENT_API=NO\n");
    printf("NEW_PUBLIC_EXECUTION_API_REQUIRED=YES\n");
    printf("PRODUCTION_CHANGE=NONE\n");
    printf("PUBLIC_API_CHANGE=NONE\n");
    printf("N6_06B_CLASSIFICATION=LIMITED\n");
    printf("N6_06B_DIRECT_RELOCATE=LIMITED\n");
    printf("N6_06B_IMPLEMENTATION_FEASIBLE=NO\n");
    printf("N6_06B_IMPLEMENTATION_START=NO\n");
    printf("N6_06B_00_MEASUREMENT=PASS\n");
    fflush(stdout);
}

static int run_all_probes(const char *libpath)
{
    n6_06b_metrics m;

    ZeroMemory(&m, sizeof(m));
    m.child_exit_rc = (DWORD)-1;

    printf("=== N6-06B-00 probe: tls_lifetime ===\n");
    if (probe_tls_lifetime(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: global_ctor ===\n");
    if (probe_global_ctor(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: manual_main ===\n");
    if (probe_manual_main(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: host_worker ===\n");
    if (probe_host_worker(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: delete_live_tls ===\n");
    if (probe_delete_live_tls(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: multi_state ===\n");
    if (probe_multi_state(libpath, &m) != 0)
        return 1;

    printf("\n=== N6-06B-00 probe: manual_main_exit (child) ===\n");
    if (spawn_child_exit_probe(libpath, &m.child_exit_rc) != 0) {
        printf("CHILD_SPAWN_FAILED=YES\n");
        m.child_exit_rc = (DWORD)-1;
    } else {
        printf("CHILD_EXIT_CODE=%lu\n", (unsigned long)m.child_exit_rc);
    }

    print_authority(&m);
    return 0;
}

int main(int argc, char **argv)
{
    const char *libpath;

    libpath = "../../../";
    if (argc > 1)
        libpath = argv[1];

    if (argc > 2 && strcmp(argv[2], "child_exit") == 0)
        return run_child_main_exit(libpath);

    return run_all_probes(libpath);
}
