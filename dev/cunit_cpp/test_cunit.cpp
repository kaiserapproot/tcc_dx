/*
 *  CUnit - A Unit testing framework library for C.
 *  Copyright (C) 2004-2006  Jerry St.Clair
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Support for unit tests of CUnit framework
 *
 *  12-Aug-2004   Initial implementation. (JDS)
 *
 *  02-May-2006   Added internationalization hooks.  (JDS)
 */

/** @file
 * CUnit internal testingfunctions (implementation).
 */
/** @addtogroup Internal
 @{
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CUNIT_BUILD_TESTS
#include "CUnit.h"
#include "MyMem.h"
#include "Util.h"
#include "CUnit_intl.h"
#include "test_cunit.h"

/* ---- Stage 3 (C++ 化): ファイルスコープの static 変数群を
   クラスのメンバに置き換え、集計処理をメンバ関数にした。
   外部 API (extern "C") のシグネチャは C 版と完全に同一。 ---- */

class TestCounter {
private:
    unsigned int nTests;
    unsigned int nFailures;
    unsigned int nTestsStored;
    unsigned int nFailsStored;
    clock_t      startTime;

public:
    /* コンストラクタは定義しない。静的記憶域のオブジェクトでは
       コンストラクタが呼ばれない（PE の起動コードが .init_array を
       実行しないため）ので、初期化は initialize() で明示的に行う。
       静的記憶域はゼロ初期化されるため未初期化参照にはならない。 */
    void initialize() {
        nTests = 0;
        nFailures = 0;
        startTime = clock();
    }

    void addTest() { nTests = nTests + 1; }
    void addFailure() { nFailures = nFailures + 1; }

    unsigned int testCount() { return nTests; }
    unsigned int failureCount() { return nFailures; }
    unsigned int successCount() { return nTests - nFailures; }

    void markStart() {
        nTestsStored = nTests;
        nFailsStored = nFailures;
    }
    unsigned int testsSinceStart() { return nTests - nTestsStored; }
    unsigned int failsSinceStart() { return nFailures - nFailsStored; }

    double elapsed() {
        return ((double)clock() - (double)startTime) / (double)CLOCKS_PER_SEC;
    }
};

static TestCounter f_counter;

static void test_cunit_initialize(void);
static void test_cunit_report_results(void);

int main()
{
  /* No line buffering. */
  setbuf(stdout, NULL);

  test_cunit_initialize();
  fprintf(stdout, "\n%s", _("Testing CUnit internals..."));

	/* individual module test functions go here */
  test_cunit_CUError();
  test_cunit_MyMem();
  test_cunit_TestDB();
  test_cunit_TestRun();
  test_cunit_Util();

  test_cunit_report_results();
  CU_cleanup_registry();

	return 0;
}

void test_cunit_start_tests(const char* strName)
{
  fprintf(stdout, _("\n     testing %s ... "), strName);
  f_counter.markStart();
}

void test_cunit_end_tests(void)
{
  fprintf(stdout, _("%d assertions, %d failures"),
                  f_counter.testsSinceStart(),
                  f_counter.failsSinceStart());
}

void test_cunit_add_test(void)
{
  f_counter.addTest();
}

void test_cunit_add_failure(void)
{
  f_counter.addFailure();
}

unsigned int test_cunit_test_count(void)
{
  return f_counter.testCount();
}

unsigned int test_cunit_failure_count(void)
{
  return f_counter.failureCount();
}

void test_cunit_initialize(void)
{
  f_counter.initialize();
}

void test_cunit_report_results(void)
{
  fprintf(stdout,
          "\n\n---------------------------"
          "\n%s"
          "\n---------------------------"
          "\n  %s%d"
          "\n     %s%d"
          "\n     %s%d"
          "\n\n%s%8.3f%s\n",
          _("CUnit Internal Test Results"),
          _("Total Number of Assertions: "),
          f_counter.testCount(),
          _("Successes: "),
          f_counter.successCount(),
          _("Failures: "),
          f_counter.failureCount(),
          _("Total test time = "),
          f_counter.elapsed(),
          _(" seconds."));
}

CU_BOOL test_cunit_assert_impl(CU_BOOL value, 
                               const char* condition, 
                               const char* file, 
                               unsigned int line)
{
  test_cunit_add_test();
  if (CU_FALSE == value) {
    test_cunit_add_failure();
    printf(_("\nTEST FAILED: File '%s', Line %d, Condition '%s.'\n"),
           file, line, condition);
  }
  return value;
}



