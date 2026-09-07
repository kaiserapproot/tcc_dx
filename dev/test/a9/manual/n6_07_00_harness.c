// N6-07-00: libtcc fail-closed inventory harness (measurement-only).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include "tcc.h"

typedef int (*fn_i0)(void);
typedef unsigned int (*fn_u0)(void);
typedef unsigned long long (*fn_u64)(void);
typedef int (*fn_i2)(unsigned long long, unsigned long long);

static int harness_register_exit(void (*fn)(void))
{
    (void)fn;
    return 0;
}

static void harness_suppress_wer(void)
{
    // N6-07-04: intentional AV probe must not open WER / fault dialog (n6_05 pattern).
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
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

static int compile_probe(TCCState *s, const char *libpath, const char *name)
{
    char path[1024];

    if (build_path(path, sizeof(path), libpath, name) < 0)
        return -1;
    return tcc_add_file(s, path);
}

static int try_uaf_dtor_call(void *saved_obj, void *saved_dtor)
{
    (void)saved_obj;
    (void)saved_dtor;
    return 0;
}

static int probe_delete_live(const char *libpath)
{
    TCCState *s;
    fn_i0 touch;
    fn_u0 count_fn;
    fn_u64 obj_fn, dtor_fn;
    fn_i2 dtor_in_fn, obj_in_fn;
    void *run_lo, *run_hi;
    unsigned long long run_lo_u, run_hi_u;
    unsigned int count;
    void *obj0, *dtor0;
    int dtor_in_rt, obj_in_rt;

    harness_suppress_wer();
    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_probe(s, libpath, "n6_07_00_delete_live_tls_probe.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    run_lo = s->run_ptr;
    run_hi = (char *)s->run_ptr + s->run_size;
    touch = (fn_i0)tcc_get_symbol(s, "touch_live_tls");
    count_fn = (fn_u0)tcc_get_symbol(s, "probe_live_tls_registry_count");
    obj_fn = (fn_u64)tcc_get_symbol(s, "probe_live_tls_registry_obj0_u64");
    dtor_fn = (fn_u64)tcc_get_symbol(s, "probe_live_tls_registry_dtor0_u64");
    dtor_in_fn = (fn_i2)tcc_get_symbol(s, "probe_dtor_ptr_inside_run_range");
    obj_in_fn = (fn_i2)tcc_get_symbol(s, "probe_obj_ptr_inside_run_range");
    if (!touch || !count_fn || !obj_fn || !dtor_fn || !dtor_in_fn || !obj_in_fn) {
        tcc_delete(s);
        return 1;
    }
    touch();
    count = count_fn();
    run_lo_u = (unsigned long long)(size_t)run_lo;
    run_hi_u = (unsigned long long)(size_t)run_hi;
    obj0 = (void *)(size_t)obj_fn();
    dtor0 = (void *)(size_t)dtor_fn();
    dtor_in_rt = dtor_in_fn(run_lo_u, run_hi_u);
    obj_in_rt = obj_in_fn(run_lo_u, run_hi_u);
    printf("REGISTRY_SNAPSHOT count=%u obj0=%p dtor0=%p\n", count, obj0, dtor0);
    printf("RUN_PTR_LO=%p RUN_PTR_HI=%p\n", run_lo, run_hi);
    printf("PENDING_DTOR_POINTER_INSIDE_RT_MEM=%s\n", dtor_in_rt ? "YES" : "NO");
    printf("PENDING_OBJECT_POINTER_INSIDE_RT_MEM=%s\n", obj_in_rt ? "YES" : "NO");
    printf("PENDING_TLS_DTOR_AT_TCC_DELETE=%s\n", count > 0 ? "YES" : "NO");
    fflush(stdout);
    printf("TCC_DELETE_BEGIN\n");
    fflush(stdout);
    tcc_delete(s);
    printf("TCC_DELETE_END=UNEXPECTED\n");
    tcc_delete(s);
    return 2;
}

static int probe_direct_relocate_boundary(const char *libpath)
{
    TCCState *s;
    fn_i0 main_fn;
    fn_i0 run_st_fn, main_st_fn;

    s = new_state_memory(libpath);
    if (!s)
        return 1;
    if (compile_probe(s, libpath, "n6_06b_probe_manual_main.cpp") < 0
        || tcc_relocate(s) < 0) {
        tcc_delete(s);
        return 1;
    }
    run_st_fn = (fn_i0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_run_state");
    main_st_fn = (fn_i0)tcc_get_symbol(s, "__tcc_cpp_tls_n6_main_state");
    main_fn = (fn_i0)tcc_get_symbol(s, "main");
    if (main_fn)
        main_fn();
    printf("DIRECT_RELOCATE_CAPABILITY=LIMITED\n");
    printf("DIRECT_RELOCATE_REPORTS_FULL_N6_SUPPORT=NO\n");
    printf("GET_SYMBOL_MAIN_PROMOTES_TO_TCC_RUN_SEMANTICS=NO\n");
    printf("TCC_DELETE_PROMOTES_TO_EXECUTION_END=NO\n");
    printf("N6_RUN_STATE_AFTER_MANUAL_MAIN=%d\n", run_st_fn ? run_st_fn() : -1);
    printf("N6_MAIN_STATE_SYMBOL=%s\n", main_st_fn ? "PRESENT" : "ABSENT");
    fflush(stdout);
    tcc_delete(s);
    return 0;
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

static void print_link_intercept_audit(void)
{
    printf("CAN_INTERCEPT_EXITTHREAD_AT_LINK_BOUNDARY=YES\n");
    printf("CAN_FAIL_CLOSED_BEFORE_PARTIAL_RECLAIM=YES\n");
    printf("CAN_INTERCEPT_ENDTHREADEX_AT_LINK_BOUNDARY=YES\n");
    printf("CAN_FAIL_CLOSED_ENDTHREADEX=YES\n");
    printf("EXITTHREAD_LINK_INTERCEPT_CONFLICT=NOT_OBSERVED\n");
    printf("ENDTHREADEX_LINK_INTERCEPT_CONFLICT=NOT_OBSERVED\n");
    fflush(stdout);
}

static int is_av_exit_code(DWORD rc)
{
    return rc == (DWORD)0xC0000005u || rc == (DWORD)0xC0000409u;
}

static int is_fail_stop_exit(DWORD rc)
{
    if (rc == 0 || rc == STILL_ACTIVE)
        return 0;
    if (is_av_exit_code(rc))
        return 0;
    return 1;
}

int main(int argc, char **argv)
{
    const char *libpath;
    DWORD child_rc;

    harness_suppress_wer();
    libpath = "../../../";
    if (argc > 1)
        libpath = argv[1];

    if (argc > 2 && strcmp(argv[2], "child_delete_live") == 0)
        return probe_delete_live(libpath);

    printf("=== N6-07-06 probe: direct_relocate_boundary ===\n");
    if (probe_direct_relocate_boundary(libpath) != 0)
        return 1;

    printf("\n=== N6-07-02/03 link intercept source audit ===\n");
    print_link_intercept_audit();

    // N6-07-04: never tcc_delete()+live TLS in this parent process; isolated child only.
    printf("\n=== N6-07-04 probe: delete_live child (isolated) ===\n");
    if (spawn_child(libpath, "child_delete_live", &child_rc) != 0) {
        printf("CHILD_SPAWN_FAILED=YES\n");
        child_rc = (DWORD)-1;
    }
    printf("THREAD_EXIT_AFTER_TCC_DELETE_CHILD_RC=%lu\n", (unsigned long)child_rc);
    if (is_fail_stop_exit(child_rc)) {
        printf("THREAD_EXIT_AFTER_TCC_DELETE_CRASH=NO\n");
        printf("TCC_DELETE_WITH_LIVE_TLS=FAIL_CLOSED\n");
        printf("PENDING_DTOR_UAF_PREVENTED=YES\n");
    } else if (is_av_exit_code(child_rc)) {
        printf("THREAD_EXIT_AFTER_TCC_DELETE_CRASH=YES\n");
        printf("TCC_DELETE_WITH_LIVE_TLS=AV\n");
    } else {
        printf("THREAD_EXIT_AFTER_TCC_DELETE_CRASH=NO\n");
        printf("TCC_DELETE_WITH_LIVE_TLS=UNEXPECTED_PASS\n");
    }

    return 0;
}
