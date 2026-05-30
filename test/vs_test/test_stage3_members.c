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

void test_stage3_member_call_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a7\\member_call.cpp"));
}

void test_stage3_default_arg_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a7\\default_arg.cpp"));
}

void test_stage3_inline_member_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a7\\inline_member.cpp"));
}

void test_stage3_typedef_class_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a7\\typedef_class.cpp"));
}

void test_stage3_class_inline_body_compile(void)
{
    CU_ASSERT_EQUAL(0, run_tcc_compile("..\\..\\..\\..\\dev\\test\\a5\\class_inline_body.cpp"));
}

static CU_TestInfo tests_stage3[] = {
    { "member_call.cpp compiles", test_stage3_member_call_compile },
    { "default_arg.cpp compiles", test_stage3_default_arg_compile },
    { "inline_member.cpp compiles", test_stage3_inline_member_compile },
    { "typedef_class.cpp compiles", test_stage3_typedef_class_compile },
    { "class_inline_body.cpp compiles", test_stage3_class_inline_body_compile },
    CU_TEST_INFO_NULL,
};

CU_SuiteInfo stage3_suites[] = {
    TEST_SUITE_ENTRY("Stage3 TCC compile", NULL, NULL, tests_stage3),
    CU_SUITE_INFO_NULL,
};