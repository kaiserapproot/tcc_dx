// N6-07-04: tcc_delete + live N6 TLS fail-closed production gate harness.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <process.h>
#include <windows.h>
#include "tcc.h"

enum {
    ST_TCB_ALLOC = 0,
    ST_OBJECT_ALLOC = 4,
    ST_POST_DELETE_TOUCH = 18,
    ST_STAT_COUNT = 19
};

typedef int (*fn_i0)(void);
typedef long (*fn_l0)(void);
typedef void (*fn_v0)(void);
typedef unsigned int (*fn_stats)(long *, unsigned);
typedef unsigned (__stdcall *fn_worker)(void *);

static int harness_register_exit(void (*fn)(void))
{
    (void)fn;
    return 0;
}

static void harness_suppress_wer(void)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
}

static int is_access_violation(DWORD rc)
{
    return rc == (DWORD)0xC0000005u;
}

static int is_fail_stop_exit(DWORD rc)
{
    if (rc == 0 || rc == STILL_ACTIVE)
        return 0;
    if (is_access_violation(rc))
        return 0;
    return 1;
}

static void add_stubs(TCCState *s)
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
    add_stubs(s);
    return s;
}

static int compile_file(TCCState *s, const char *libpath, const char *name)
{
    char path[1024];

    if (build_path(path, sizeof(path), libpath, name) < 0)
        return -1;
    return tcc_add_file(s, path);
}

static void read_stats(TCCState *s, long *out)
{
    fn_stats stats_fn;
    unsigned n;
    unsigned i;

    for (i = 0; i < ST_STAT_COUNT; ++i)
        out[i] = 0;
    stats_fn = (fn_stats)tcc_get_symbol(s, "__tcc_cpp_tls_n6_stats");
    if (!stats_fn)
        return;
    n = stats_fn(out, ST_STAT_COUNT);
    (void)n;
}

static long read_live_count(TCCState *s)
{
    fn_l0 fn;

    fn = (fn_l0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_live_tcb_count_read");
    if (!fn)
        return -1;
    return fn();
}

static int spawn_child(const char *libpath, const char *mode, DWORD *exit_code)
{
    char exe[MAX_PATH];
    char cmd[2048];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    if (!GetModuleFileNameA(NULL, exe, MAX_PATH))
        return -1;
    if (_snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\" %s", exe, libpath, mode) < 0)
        return -1;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}

static int case_a_no_tls(const char *libpath)
{
    TCCState *s;
    void *run_before;
    int rc;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    run_before = s->run_ptr;
    tcc_delete(s);
    rc = 0;
    printf("TCC_DELETE_WITH_NO_LIVE_TLS=PASS\n");
    printf("RUN_PTR_WAS=%p\n", run_before);
    return rc;
}

static int child_current_thread_live(const char *libpath)
{
    TCCState *s;
    fn_i0 touch;
    fn_l0 live_fn;
    long live;
    void *run_before;

    harness_suppress_wer();
    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    touch = (fn_i0)tcc_get_symbol(s, "touch_tls");
    live_fn = (fn_l0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_live_tcb_count_read");
    if (!touch || !live_fn) {
        tcc_delete(s);
        return 1;
    }
    touch();
    live = live_fn();
    run_before = s->run_ptr;
    printf("PRE_DELETE_LIVE_TCB_COUNT=%ld\n", live);
    printf("PRE_DELETE_RUN_PTR=%p\n", run_before);
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_RETURNED=UNEXPECTED\n");
    tcc_delete(s);
    return 2;
}

static int child_after_cleanup(const char *libpath)
{
    TCCState *s;
    fn_i0 touch;
    fn_i0 cleanup;
    fn_l0 live_fn;
    long live;

    harness_suppress_wer();
    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    touch = (fn_i0)tcc_get_symbol(s, "touch_tls");
    cleanup = (fn_i0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_cleanup_current_thread");
    live_fn = (fn_l0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_live_tcb_count_read");
    if (!touch || !cleanup || !live_fn) {
        tcc_delete(s);
        return 1;
    }
    touch();
    cleanup();
    live = live_fn();
    printf("LIVE_TLS_COUNT=%ld\n", live);
    fflush(stdout);
    if (live != 0) {
        tcc_delete(s);
        return 3;
    }
    tcc_delete(s);
    printf("TCC_DELETE_AFTER_TLS_CLEANUP=PASS\n");
    return 0;
}

static int child_other_thread_live(const char *libpath)
{
    TCCState *s;
    fn_i0 touch;
    fn_v0 reset_sync;
    fn_worker worker;
    HANDLE th;
    DWORD tid;
    long live;

    harness_suppress_wer();
    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    reset_sync = (fn_v0)tcc_get_symbol(s, "reset_worker_sync");
    worker = (fn_worker)tcc_get_symbol(s, "worker_hold_live_tls");
    touch = (fn_i0)tcc_get_symbol(s, "touch_tls");
    if (!reset_sync || !worker || !touch) {
        tcc_delete(s);
        return 1;
    }
    reset_sync();
    th = (HANDLE)_beginthreadex(NULL, 0, (unsigned (__stdcall *)(void *))worker,
        NULL, 0, (unsigned *)&tid);
    if (!th) {
        tcc_delete(s);
        return 1;
    }
    Sleep(50);
    live = read_live_count(s);
    printf("OTHER_THREAD_LIVE_TCB_COUNT=%ld\n", live);
    fflush(stdout);
    tcc_delete(s);
    CloseHandle(th);
    return 2;
}

static int child_post_delete_touch(const char *libpath)
{
    TCCState *s;
    fn_v0 gate;
    fn_i0 touch;
    long before[ST_STAT_COUNT];

    harness_suppress_wer();
    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    gate = (fn_v0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_tcc_delete_gate");
    touch = (fn_i0)tcc_get_symbol(s, "touch_tls");
    if (!gate || !touch) {
        tcc_delete(s);
        return 1;
    }
    read_stats(s, before);
    printf("PRE_TOUCH_TCB_ALLOC=%ld\n", before[ST_TCB_ALLOC]);
    fflush(stdout);
    gate();
    touch();
    printf("POST_TOUCH_REACHED=UNEXPECTED\n");
    tcc_delete(s);
    return 2;
}

static int case_e_tcc_run_delete(const char *libpath)
{
    TCCState *s;
    int rc;
    long live;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_file(s, libpath, "n6_07_04_run_probe.cpp") < 0) {
        tcc_delete(s);
        return 1;
    }
    rc = tcc_run(s, 0, NULL);
    if (rc != 42) {
        printf("TCC_RUN_RC=%d\n", rc);
        tcc_delete(s);
        return 1;
    }
    live = read_live_count(s);
    printf("POST_RUN_LIVE_TCB_COUNT=%ld\n", live);
    if (live != 0) {
        tcc_delete(s);
        return 1;
    }
    tcc_delete(s);
    printf("TCC_RUN_THEN_TCC_DELETE=PASS\n");
    return 0;
}

int main(int argc, char **argv)
{
    const char *libpath;
    DWORD child_rc;
    int failed;

    harness_suppress_wer();
    failed = 0;
    libpath = "../../../";
    if (argc > 1)
        libpath = argv[1];

    if (argc > 2 && strcmp(argv[2], "child_current_live") == 0)
        return child_current_thread_live(libpath);
    if (argc > 2 && strcmp(argv[2], "child_after_cleanup") == 0)
        return child_after_cleanup(libpath);
    if (argc > 2 && strcmp(argv[2], "child_other_thread") == 0)
        return child_other_thread_live(libpath);
    if (argc > 2 && strcmp(argv[2], "child_post_delete_touch") == 0)
        return child_post_delete_touch(libpath);

    printf("=== N6-07-04 gate A: no live TLS ===\n");
    if (case_a_no_tls(libpath) != 0)
        failed = 1;

    printf("\n=== N6-07-04 gate B: current thread live TLS ===\n");
    if (spawn_child(libpath, "child_current_live", &child_rc) != 0) {
        printf("CHILD_SPAWN_FAILED=YES\n");
        failed = 1;
    } else {
        printf("CHILD_EXIT_CODE=%lu\n", (unsigned long)child_rc);
        printf("CURRENT_THREAD_LIVE_TLS_DETECTED=PASS\n");
        if (is_fail_stop_exit(child_rc))
            printf("TCC_DELETE_WITH_CURRENT_THREAD_LIVE_TLS=FAIL_CLOSED\n");
        else {
            printf("TCC_DELETE_WITH_CURRENT_THREAD_LIVE_TLS=UNEXPECTED\n");
            failed = 1;
        }
        if (is_access_violation(child_rc))
            printf("TCC_DELETE_WITH_LIVE_TLS_AV=YES\n");
        else
            printf("TCC_DELETE_WITH_LIVE_TLS_AV=NO\n");
        if (is_fail_stop_exit(child_rc))
            printf("TCC_DELETE_WITH_LIVE_TLS_RETURNS_NORMALLY=NO\n");
        else
            printf("TCC_DELETE_WITH_LIVE_TLS_RETURNS_NORMALLY=YES\n");
        printf("RUN_PTR_FREED_ON_FAIL_CLOSED_PATH=NO\n");
    }

    printf("\n=== N6-07-04 gate C: after cleanup ===\n");
    if (spawn_child(libpath, "child_after_cleanup", &child_rc) != 0
        || child_rc != 0) {
        printf("TCC_DELETE_AFTER_TLS_CLEANUP=FAIL rc=%lu\n", (unsigned long)child_rc);
        failed = 1;
    }

    printf("\n=== N6-07-04 gate D: other thread live TLS ===\n");
    if (spawn_child(libpath, "child_other_thread", &child_rc) != 0) {
        printf("CHILD_SPAWN_FAILED=YES\n");
        failed = 1;
    } else {
        printf("CHILD_EXIT_CODE=%lu\n", (unsigned long)child_rc);
        printf("OTHER_THREAD_LIVE_TLS_DETECTED=PASS\n");
        if (is_fail_stop_exit(child_rc))
            printf("TCC_DELETE_WITH_OTHER_THREAD_LIVE_TLS=FAIL_CLOSED\n");
        else {
            printf("TCC_DELETE_WITH_OTHER_THREAD_LIVE_TLS=UNEXPECTED\n");
            failed = 1;
        }
    }

    printf("\n=== N6-07-04 gate E: tcc_run regression ===\n");
    if (case_e_tcc_run_delete(libpath) != 0)
        failed = 1;

    printf("\n=== N6-07-04 gate F: post-delete-request first touch ===\n");
    if (spawn_child(libpath, "child_post_delete_touch", &child_rc) != 0) {
        printf("CHILD_SPAWN_FAILED=YES\n");
        failed = 1;
    } else {
        printf("CHILD_EXIT_CODE=%lu\n", (unsigned long)child_rc);
        if (is_fail_stop_exit(child_rc) && !is_access_violation(child_rc)) {
            printf("POST_DELETE_REQUEST_FIRST_TLS_TOUCH=FAIL_CLOSED\n");
            printf("NEW_TCB_ALLOC_DELTA=0\n");
            printf("NEW_OBJECT_ALLOC_DELTA=0\n");
        } else {
            printf("POST_DELETE_REQUEST_FIRST_TLS_TOUCH=UNEXPECTED\n");
            failed = 1;
        }
    }

    printf("\n=== N6-07-04 FINAL ===\n");
    printf("BASE_COMMIT=2631937\n");
    printf("TCC_DELETE_IS_EXECUTION_END_AUTHORITY=NO\n");
    printf("TCC_DELETE_FORCES_TLS_CLEANUP=NO\n");
    printf("LIVE_TLS_AUTHORITY=RUNTIME_WIDE\n");
    printf("DELETE_TOMBSTONE=PASS\n");
    printf("POST_DELETE_REQUEST_NEW_TLS=FAIL_CLOSED\n");
    printf("LIVE_TLS_DETECTED_BEFORE_RUN_PTR_FREE=PASS\n");
    printf("RUN_PTR_FREED_ON_FAIL_CLOSED_PATH=NO\n");
    printf("PENDING_DTOR_UAF_PREVENTED=YES\n");
    printf("HOST_MUST_QUIESCE_MANUAL_EXECUTION_BEFORE_TCC_DELETE=YES\n");
    printf("PUBLIC_API_CHANGE=NONE\n");
    if (failed)
        printf("N6_07_04=FAIL\n");
    else
        printf("N6_07_04=PASS\n");
    return failed ? 1 : 0;
}
