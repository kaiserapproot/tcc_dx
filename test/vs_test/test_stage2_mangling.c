#include "test_common.h"
#include <stdlib.h>
#include <stdio.h>

static int run_tcc_compile(const char *src_rel)
{
    char cmd[1024];
    _snprintf(cmd, sizeof cmd,
        "..\\..\\..\\..\\dev\\tcc.exe -c %s -o nul 2>nul", src_rel);
    return system(cmd);
}

void test_stage2_overload_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a6\\overload.cpp"));
}

void test_stage2_ref_param_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a6\\ref_param.cpp"));
}

void test_stage2_qualified_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a6\\qualified.cpp"));
}

static CU_TestInfo tests_stage2[] = {
    { "overload.cpp compiles", test_stage2_overload_compile },
    { "ref_param.cpp compiles", test_stage2_ref_param_compile },
    { "qualified.cpp compiles", test_stage2_qualified_compile },
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo stage2_suites[] = {
    TEST_SUITE_ENTRY("Stage2 TCC compile", NULL, NULL, tests_stage2),
    CU_SUITE_INFO_NULL,
};
