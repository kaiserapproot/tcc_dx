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

void test_stage4_inherit_single_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a8\\inherit_single.cpp"));
}

void test_stage4_inherit_method_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a8\\inherit_method.cpp"));
}

void test_stage4_static_member_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a8\\static_member.cpp"));
}

void test_stage4_inherit_sizeof_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a8\\inherit_sizeof.cpp"));
}

static CU_TestInfo tests_stage4[] = {
    { "inherit_single.cpp compiles", test_stage4_inherit_single_compile },
    { "inherit_method.cpp compiles", test_stage4_inherit_method_compile },
    { "inherit_sizeof.cpp compiles", test_stage4_inherit_sizeof_compile },
    { "static_member.cpp compiles", test_stage4_static_member_compile },
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo stage4_suites[] = {
    TEST_SUITE_ENTRY("Stage4 TCC compile", NULL, NULL, tests_stage4),
    CU_SUITE_INFO_NULL,
};