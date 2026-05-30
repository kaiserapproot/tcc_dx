#include "test_common.h"
extern CU_SuiteInfo stage2_suites[];
extern CU_SuiteInfo stage3_suites[];
extern CU_SuiteInfo stage4_suites[];
#include <Basic.h>

void test_no_cplusplus_macro(void)
{
#ifdef __cplusplus
    CU_FAIL("__cplusplus should not be defined in C mode");
#else
    CU_PASS();
#endif
}

void test_class_as_identifier(void)
{
    int class = 42;
    CU_ASSERT_EQUAL(42, class);
}

static CU_TestInfo tests_foundation[] = {
    { "no __cplusplus in C mode", test_no_cplusplus_macro },
    { "class as identifier",      test_class_as_identifier },
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites[] = {
    { "C Foundation Tests", NULL, NULL, tests_foundation },
    CU_SUITE_INFO_NULL,
};

void *tinyc_getbp(void) { return NULL; }

int main(void)
{
    CU_initialize_registry();
    CU_register_suites(suites);
    CU_register_suites(stage2_suites);
    CU_register_suites(stage3_suites);
    CU_register_suites(stage4_suites);
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return 0;
}
