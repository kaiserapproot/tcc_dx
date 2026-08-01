#include <CUnit.h>
#include <Basic.h>
int car_setup(void) {
	int c = 0;
	return c != NULL ? 0 : 1;
}

int car_teardown(void) {
	return 0;
}

void test_car_init(void) {
	CU_ASSERT_EQUAL(1, 0);
	CU_ASSERT_EQUAL(1, 0);
}

void test_car_refuel(void) {
	CU_ASSERT_EQUAL(1, 20);
	CU_ASSERT_EQUAL(1, 40);
	CU_ASSERT_EQUAL(1, 50);
}

static CU_TestInfo test_car[] = {
  { "初期化", test_car_init },
  { "給油",   test_car_refuel },
  CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites[] = {
  { "Carのテスト",  car_setup, car_teardown, test_car },
  CU_SUITE_INFO_NULL,
};
void *tinyc_getbp(void)
{
    return NULL; // 実際のスタックトレースは行わないシンプルな実装
}
int main()
{
	CU_initialize_registry();
	CU_register_suites(suites);
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
}