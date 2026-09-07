// N6-06A libtcc harness: cross-TCCState tombstone isolation (safe default path).
// Contract probe (same-TCCState 2x tcc_run) is opt-in: second arg "contract".
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtcc.h"

static int exec_once_new_state(const char *libpath, int *out_rc)
{
    TCCState *s;
    char srcpath[1024];
    int rc;

    s = tcc_new();
    if (!s)
        return -1;
    tcc_set_lib_path(s, libpath);
    if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) < 0) {
        tcc_delete(s);
        return -1;
    }
    if (_snprintf(srcpath, sizeof(srcpath), "%s/test/a9/manual/n6_06a_second_epoch.cpp", libpath) < 0) {
        tcc_delete(s);
        return -1;
    }
    if (tcc_add_file(s, srcpath) < 0) {
        tcc_delete(s);
        return -1;
    }
    rc = tcc_run(s, 0, NULL);
    tcc_delete(s);
    if (out_rc)
        *out_rc = rc;
    return 0;
}

static int run_cross_tccstate(const char *libpath)
{
    int r1, r2;

    if (exec_once_new_state(libpath, &r1) != 0 || exec_once_new_state(libpath, &r2) != 0) {
        printf("N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=HARNESS_FAIL\n");
        return 1;
    }
    if (r1 != 110 || r2 != 110) {
        printf("N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=RC_FAIL r1=%d r2=%d\n", r1, r2);
        return 2;
    }
    printf("N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=PASS\n");
    printf("N6_06A_SECOND_EXECUTION_SAME_PROCESS=PASS\n");
    printf("N6_06A_SECOND_EXECUTION_SAME_HOST_THREAD=PASS\n");
    return 0;
}

// Same-TCCState 2nd tcc_run must fail closed (rc=-1), never AV (N6-06A one-shot).
static int run_same_tccstate_contract(const char *libpath)
{
    TCCState *s;
    char srcpath[1024];
    int r1, r2;

    s = tcc_new();
    if (!s)
        return 1;
    tcc_set_lib_path(s, libpath);
    if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) < 0) {
        tcc_delete(s);
        return 1;
    }
    if (_snprintf(srcpath, sizeof(srcpath), "%s/test/a9/manual/n6_06a_second_epoch.cpp", libpath) < 0) {
        tcc_delete(s);
        return 1;
    }
    if (tcc_add_file(s, srcpath) < 0) {
        tcc_delete(s);
        return 1;
    }
    r1 = tcc_run(s, 0, NULL);
    r2 = tcc_run(s, 0, NULL);
    tcc_delete(s);

    printf("SAME_TCCSTATE_SECOND_TCC_RUN_ATTEMPTED=YES\n");
    printf("SAME_TCCSTATE_RUN1_RC=%d\n", r1);
    printf("SAME_TCCSTATE_RUN2_RC=%d\n", r2);
    printf("TCC_RUN_EXECUTIONS_PER_RUNTIME_IMAGE=ONE\n");
    printf("N6_06A_SAME_RUNTIME_IMAGE_SECOND_EPOCH=UNSUPPORTED\n");
    printf("N6_06A_FINALIZED_TO_NEXT_EPOCH_TRANSITION=NOT_SUPPORTED\n");

    if (r1 != 110) {
        printf("SAME_TCCSTATE_SECOND_TCC_RUN=UNEXPECTED\n");
        printf("SAME_TCCSTATE_SECOND_TCC_RUN_CRASH=UNKNOWN\n");
        return 2;
    }
    if (r2 == -1) {
        printf("SAME_TCCSTATE_SECOND_TCC_RUN=UNSUPPORTED_FAIL_CLOSED\n");
        printf("SAME_TCCSTATE_SECOND_TCC_RUN_CRASH=NO\n");
        return 0;
    }
    printf("SAME_TCCSTATE_SECOND_TCC_RUN=UNEXPECTED\n");
    printf("SAME_TCCSTATE_SECOND_TCC_RUN_CRASH=UNKNOWN\n");
    return 3;
}

int main(int argc, char **argv)
{
    const char *libpath;
    int mode_contract;

    libpath = "../../../";
    mode_contract = 0;
    if (argc > 1)
        libpath = argv[1];
    if (argc > 2 && strcmp(argv[2], "contract") == 0)
        mode_contract = 1;

    if (mode_contract)
        return run_same_tccstate_contract(libpath);
    return run_cross_tccstate(libpath);
}
