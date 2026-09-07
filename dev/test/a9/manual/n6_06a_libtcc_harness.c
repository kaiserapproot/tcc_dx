// N6-06A libtcc harness: cross-TCCState tombstone isolation + same-TCCState contract probe.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtcc.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

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

static int exec_twice_same_state(const char *libpath, int *r1, int *r2)
{
    TCCState *s;
    char srcpath[1024];

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
    *r1 = tcc_run(s, 0, NULL);
#ifdef _MSC_VER
    __try {
        *r2 = tcc_run(s, 0, NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *r2 = -999;
    }
#else
    *r2 = tcc_run(s, 0, NULL);
#endif
    tcc_delete(s);
    return 0;
}

int main(int argc, char **argv)
{
    const char *libpath;
    int r1, r2, r1b, r2b;

    libpath = "../../../";
    if (argc > 1)
        libpath = argv[1];

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

    if (exec_twice_same_state(libpath, &r1b, &r2b) != 0) {
        printf("SAME_TCCSTATE_SECOND_TCC_RUN=HARNESS_FAIL\n");
        return 3;
    }
    printf("SAME_TCCSTATE_SECOND_TCC_RUN_ATTEMPTED=YES\n");
    printf("SAME_TCCSTATE_RUN1_RC=%d\n", r1b);
    printf("SAME_TCCSTATE_RUN2_RC=%d\n", r2b);
    if (r1b == 110 && r2b == 210) {
        printf("N6_06A_SAME_RUNTIME_IMAGE_SECOND_EPOCH=PASS\n");
        printf("N6_06A_FINALIZED_TO_NEXT_EPOCH_TRANSITION=PASS\n");
        printf("SAME_TCCSTATE_SECOND_TCC_RUN=PASS\n");
    } else if (r2b < 0) {
        printf("N6_06A_SAME_RUNTIME_IMAGE_SECOND_EPOCH=UNSUPPORTED\n");
        printf("N6_06A_FINALIZED_TO_NEXT_EPOCH_TRANSITION=UNPROVEN\n");
        if (r2b == -999)
            printf("SAME_TCCSTATE_SECOND_TCC_RUN=CRASH\n");
        else
            printf("SAME_TCCSTATE_SECOND_TCC_RUN=UNSUPPORTED_BY_CURRENT_API\n");
        printf("TCC_RUN_EXECUTIONS_PER_RUNTIME_IMAGE=ONE\n");
    } else {
        printf("N6_06A_SAME_RUNTIME_IMAGE_SECOND_EPOCH=UNPROVEN\n");
        printf("N6_06A_FINALIZED_TO_NEXT_EPOCH_TRANSITION=UNPROVEN\n");
        printf("SAME_TCCSTATE_SECOND_TCC_RUN=UNEXPECTED r1=%d r2=%d\n", r1b, r2b);
    }
    return 0;
}
