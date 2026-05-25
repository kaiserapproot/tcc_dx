#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include <CUnit.h>

#define TEST_SUITE_ENTRY(name, setup, teardown, tests) \
  { (name), (setup), (teardown), (tests) }

#endif
