// all_test_tpp.cpp - tpp driver for the CPPUnit library (G7 build gate).
//
// Plan A (cppunit taiou plan section 7.4): the original all_test.cpp is
// out of scope (exceptions, console color, pointer-to-member TEST_CASE).
// This driver uses only the implemented C++ subset: TestCase subclasses
// override runTest() directly (no member-function pointers), no
// exceptions, TEST_ASSERT / TEST_FAIL / TEST_ERROR only.
//
// Contract (N = 17, fixed by the coverage table in the plan):
//  - every test prints exactly one marker line "PASS:<name>" on success
//  - the run summary prints TESTS:<n> / FAILURES:<n> / ERRORS:<n>
//  - exit 0 only when n == 17 and failures == errors == 0
#include <stdio.h>
#include <string.h>

#include "TestCase.h"
#include "TestResult.h"
#include "TestSuite.h"
#include "TestRegistry.h"
#include "TestRunner.h"
#include "RepeatedTest.h"
#include "TestDecorator.h"
#include "TestSetup.h"
#include "TestListener.h"
#include "TestFailure.h"
#include "SimpleString.h"
#include "SimpleList.h"
#include "SimpleAutoPtr.h"

// ------------------------------------------------------------------
// shared helpers
// ------------------------------------------------------------------

// order/event log shared by the lifecycle-style tests
static char g_log[64];
static int g_log_len = 0;

static void log_reset()
{
    g_log_len = 0;
    g_log[0] = 0;
}

static void log_add(char c)
{
    if (g_log_len < 63) {
        g_log[g_log_len++] = c;
        g_log[g_log_len] = 0;
    }
}

// marker output; flushed so a crash cannot swallow completed PASSes
static void pass(const char *name)
{
    printf("PASS:%s\n", name);
    fflush(0);
}

// live-instance counter for ownership tests
static int g_live = 0;

// a test that always passes; counts instances and logs 'R' when run
class PassTest : public TestCase {
  public:
    PassTest(const char *name) : TestCase(name) { g_live++; }
    virtual ~PassTest() { g_live--; }
    virtual void runTest() { log_add('R'); }
};

// a test whose assertion always fails; code after the assert must not
// run (TEST_FAIL returns early)
static int g_after_fail = 0;
class FailTest : public TestCase {
  public:
    FailTest() : TestCase("helper_fail") {}
    virtual ~FailTest() {}
    virtual void runTest()
    {
        TEST_ASSERT(1 == 2);
        g_after_fail = 1;
    }
};

// a test that reports an error via TEST_ERROR
class ErrTest : public TestCase {
  public:
    ErrTest() : TestCase("helper_error") {}
    virtual ~ErrTest() {}
    virtual void runTest() { TEST_ERROR("boom"); }
};

// records the fixture call order: S(etUp) R(un) T(earDown)
class LifeTest : public TestCase {
  public:
    LifeTest() : TestCase("helper_life") {}
    virtual ~LifeTest() {}
    virtual void setUp() { log_add('S'); }
    virtual void runTest() { log_add('R'); }
    virtual void tearDown() { log_add('T'); }
};

// TEST_ASSERT_EQUALS pass / fail helpers (test #3)
static int g_eq_done = 0;
class EqTest : public TestCase {
  public:
    EqTest() : TestCase("helper_eq") {}
    virtual ~EqTest() {}
    virtual void runTest()
    {
        TEST_ASSERT_EQUALS(2, 1 + 1);
        g_eq_done = 1;
    }
};
static int g_neq_after = 0;
class NeqTest : public TestCase {
  public:
    NeqTest() : TestCase("helper_neq") {}
    virtual ~NeqTest() {}
    virtual void runTest()
    {
        TEST_ASSERT_EQUALS(3, 1 + 1);
        g_neq_after = 1;
    }
};

// dtor probes for the auto-ptr test: Probe's dtor is virtual, so
// deleting through Probe* must run ~DerivedProbe first
static int g_probe_dtor = 0;
static int g_derived_dtor = 0;
class Probe {
  public:
    Probe() {}
    virtual ~Probe() { g_probe_dtor++; }
};
class DerivedProbe : public Probe {
  public:
    DerivedProbe() {}
    virtual ~DerivedProbe() { g_derived_dtor++; }
};

// ------------------------------------------------------------------
// the 17 contract tests
// ------------------------------------------------------------------

// 1
class CaseLifecycleTest : public TestCase {
  public:
    CaseLifecycleTest() : TestCase("test_case_lifecycle") {}
    virtual ~CaseLifecycleTest() {}
    virtual void runTest()
    {
        LifeTest lt;
        TestResult r;

        log_reset();
        lt.run(&r);
        TEST_ASSERT(strcmp(g_log, "SRT") == 0);
        TEST_ASSERT(r.runCount() == 1);
        pass("test_case_lifecycle");
    }
};

// 2
class CaseFailureRecordTest : public TestCase {
  public:
    CaseFailureRecordTest() : TestCase("test_case_failure_record") {}
    virtual ~CaseFailureRecordTest() {}
    virtual void runTest()
    {
        FailTest ft;
        TestResult r;

        g_after_fail = 0;
        ft.run(&r);
        TEST_ASSERT(r.failureCount() == 1);
        TEST_ASSERT(r.errorCount() == 0);
        TEST_ASSERT(g_after_fail == 0);
        pass("test_case_failure_record");
    }
};

// 3
class AssertEqualsTest : public TestCase {
  public:
    AssertEqualsTest() : TestCase("test_assert_equals") {}
    virtual ~AssertEqualsTest() {}
    virtual void runTest()
    {
        EqTest et;
        NeqTest nt;
        TestResult r1;
        TestResult r2;

        g_eq_done = 0;
        g_neq_after = 0;
        et.run(&r1);
        TEST_ASSERT(r1.failureCount() == 0);
        TEST_ASSERT(g_eq_done == 1);
        nt.run(&r2);
        TEST_ASSERT(r2.failureCount() == 1);
        TEST_ASSERT(g_neq_after == 0);
        pass("test_assert_equals");
    }
};

// 4
class ResultCountsTest : public TestCase {
  public:
    ResultCountsTest() : TestCase("test_result_counts") {}
    virtual ~ResultCountsTest() {}
    virtual void runTest()
    {
        TestResult r;
        PassTest pt("p");
        FailTest ft;
        ErrTest et;

        TEST_ASSERT(r.wasSuccessful());
        pt.run(&r);
        ft.run(&r);
        et.run(&r);
        TEST_ASSERT(r.runCount() == 3);
        TEST_ASSERT(r.failureCount() == 1);
        TEST_ASSERT(r.errorCount() == 1);
        TEST_ASSERT(!r.wasSuccessful());
        pass("test_result_counts");
    }
};

// 5
class FailureDetailTest : public TestCase {
  public:
    FailureDetailTest() : TestCase("test_failure_detail") {}
    virtual ~FailureDetailTest() {}
    virtual void runTest()
    {
        PassTest pt("victim");
        TestFailure f(&pt, SimpleString("boom"), "afile.cpp", 42);

        TEST_ASSERT(f.failedTest() == &pt);
        TEST_ASSERT(strcmp(f.what(), "boom") == 0);
        TEST_ASSERT(strcmp(f.file(), "afile.cpp") == 0);
        TEST_ASSERT(f.line() == 42);
        pass("test_failure_detail");
    }
};

// 6
class SuiteRunTest : public TestCase {
  public:
    SuiteRunTest() : TestCase("test_suite_run") {}
    virtual ~SuiteRunTest() {}
    virtual void runTest()
    {
        TestResult r;
        int before = g_live;
        {
            TestSuite s("s");

            s.addTest(new PassTest("a"));
            s.addTest(new PassTest("b"));
            TEST_ASSERT(s.countTestCases() == 2);
            s.run(&r);
            TEST_ASSERT(r.runCount() == 2);
            TEST_ASSERT(r.failureCount() == 0);
        }
        // the suite owns and deletes its tests
        TEST_ASSERT(g_live == before);
        pass("test_suite_run");
    }
};

// 7
class RegistryTest : public TestCase {
  public:
    RegistryTest() : TestCase("test_registry") {}
    virtual ~RegistryTest() {}
    virtual void runTest()
    {
        TestResult r;
        int before = g_live;
        {
            TestRegistry reg;

            reg.addTest("a", new PassTest("a"));
            reg.addTest("b", new PassTest("b"));
            TEST_ASSERT(!reg.hasAvailables());
            TEST_ASSERT(reg.setAvailable("a"));
            TEST_ASSERT(!reg.setAvailable("zzz"));
            reg.setAllAvailable();
            TEST_ASSERT(reg.hasAvailables());
            reg.runTests(&r);
            TEST_ASSERT(r.runCount() == 2);
        }
        // Entry ownership: the registry deletes the tests
        TEST_ASSERT(g_live == before);
        pass("test_registry");
    }
};

// 8
class RunnerRunTest : public TestCase {
  public:
    RunnerRunTest() : TestCase("test_runner_run") {}
    virtual ~RunnerRunTest() {}
    virtual void runTest()
    {
        int before = g_live;
        int rc;
        {
            TestRunner tr;
            char *argv[1];

            tr.addTest(new PassTest("p"));
            argv[0] = 0;
            rc = tr.run(1, argv);
        }
        TEST_ASSERT(rc == 0);
        TEST_ASSERT(g_live == before);
        pass("test_runner_run");
    }
};

// 9
class TppRepeatedTest : public RepeatedTest {
  public:
    TppRepeatedTest(Test *test, int repeat) : RepeatedTest(test, repeat) {}
    virtual ~TppRepeatedTest() {}
};

class RepeatedRunTest : public TestCase {
  public:
    RepeatedRunTest() : TestCase("test_repeated") {}
    virtual ~RepeatedRunTest() {}
    virtual void runTest()
    {
        TestResult r;
        TppRepeatedTest rep(new PassTest("p"), 5);

        TEST_ASSERT(rep.countTestCases() == 5);
        rep.run(&r);
        TEST_ASSERT(r.runCount() == 5);
        pass("test_repeated");
    }
};

// 10
class DecoratorTest : public TestCase {
  public:
    DecoratorTest() : TestCase("test_decorator") {}
    virtual ~DecoratorTest() {}
    virtual void runTest()
    {
        TestResult r;
        TestDecorator d(new PassTest("p"));

        TEST_ASSERT(d.countTestCases() == 1);
        TEST_ASSERT(strcmp(d.getName(), "p") == 0);
        d.run(&r);
        TEST_ASSERT(r.runCount() == 1);
        pass("test_decorator");
    }
};

// 11: TestSetup's own setUp/tearDown wrap the decorated run
class MySetup : public TestSetup {
  public:
    MySetup(Test *t) : TestSetup(t) {}
    virtual ~MySetup() {}
    virtual void setUp() { log_add('['); }
    virtual void tearDown() { log_add(']'); }
};
class SetupHooksTest : public TestCase {
  public:
    SetupHooksTest() : TestCase("test_setup_hooks") {}
    virtual ~SetupHooksTest() {}
    virtual void runTest()
    {
        TestResult r;
        MySetup ms(new PassTest("p"));

        log_reset();
        ms.run(&r);
        TEST_ASSERT(strcmp(g_log, "[R]") == 0);
        pass("test_setup_hooks");
    }
};

// 12: listener notification order on a failing run
class MyListener : public TestListener {
  public:
    MyListener() {}
    virtual ~MyListener() {}
    virtual void startTest(Test *test) { log_add('s'); }
    virtual void endTest(Test *test) { log_add('e'); }
    virtual void addFailure(const TestFailure *failure) { log_add('f'); }
    virtual void addError(const TestFailure *error) { log_add('x'); }
};
class ListenerTest : public TestCase {
  public:
    ListenerTest() : TestCase("test_listener") {}
    virtual ~ListenerTest() {}
    virtual void runTest()
    {
        TestResult r;
        MyListener ml;
        FailTest ft;

        r.addListener(&ml);
        log_reset();
        ft.run(&r);
        TEST_ASSERT(strcmp(g_log, "sfe") == 0);
        pass("test_listener");
    }
};

// 13
class SimpleStringTest : public TestCase {
  public:
    SimpleStringTest() : TestCase("test_simple_string") {}
    virtual ~SimpleStringTest() {}
    virtual void runTest()
    {
        SimpleString a("abc");
        SimpleString b(a);
        SimpleString sub;

        TEST_ASSERT(a.length() == 3);
        TEST_ASSERT(a.compare("abc") == 0);
        TEST_ASSERT(b.compare(a) == 0);
        a.append("def");
        TEST_ASSERT(a.length() == 6);
        TEST_ASSERT(a.compare("abcdef") == 0);
        TEST_ASSERT(a.compare(b) != 0);
        sub = a.substr(2, 3);
        TEST_ASSERT(sub.compare("cde") == 0);
        // n = npos default argument: rest of the string
        sub = a.substr(2);
        TEST_ASSERT(sub.compare("cdef") == 0);
        TEST_ASSERT(a[1] == 'b');
        pass("test_simple_string");
    }
};

// 14: force reallocation (new value_type[n] / delete[] path)
class SimpleStringGrowTest : public TestCase {
  public:
    SimpleStringGrowTest() : TestCase("test_simple_string_grow") {}
    virtual ~SimpleStringGrowTest() {}
    virtual void runTest()
    {
        SimpleString s("x");
        int i;

        for (i = 0; i < 100; i++)
            s.append("0123456789");
        TEST_ASSERT(s.length() == 1001);
        TEST_ASSERT(s[0] == 'x');
        TEST_ASSERT(s[1] == '0');
        TEST_ASSERT(s[1000] == '9');
        pass("test_simple_string_grow");
    }
};

// 15
class SimpleListTest : public TestCase {
  public:
    SimpleListTest() : TestCase("test_simple_list") {}
    virtual ~SimpleListTest() {}
    virtual void runTest()
    {
        SimpleList l;
        int a, b, c;

        TEST_ASSERT(l.empty());
        l.push_back(&a);
        l.push_back(&b);
        TEST_ASSERT(l.size() == 2);
        TEST_ASSERT(l.front() == &a);
        TEST_ASSERT(l.back() == &b);
        l.insert(l.begin(), (void *)&c);
        TEST_ASSERT(l.size() == 3);
        TEST_ASSERT(l.front() == &c);
        l.erase(l.begin());
        TEST_ASSERT(l.size() == 2);
        TEST_ASSERT(l.front() == &a);
        l.push_front(&c);
        l.pop_back();
        TEST_ASSERT(l.size() == 2);
        TEST_ASSERT(l.back() == &a);
        l.clear();
        TEST_ASSERT(l.empty());
        pass("test_simple_list");
    }
};

// 16
class SimpleListIterTest : public TestCase {
  public:
    SimpleListIterTest() : TestCase("test_simple_list_iter") {}
    virtual ~SimpleListIterTest() {}
    virtual void runTest()
    {
        SimpleList l;
        int a, b, c;
        SimpleList::iterator it;
        int n;

        l.push_back(&a);
        l.push_back(&b);
        l.push_back(&c);
        // forward walk with pre-increment
        n = 0;
        for (it = l.begin(); it != l.end(); ++it)
            n++;
        TEST_ASSERT(n == 3);
        // deref + post-increment
        it = l.begin();
        TEST_ASSERT(*it == &a);
        it++;
        TEST_ASSERT(*it == &b);
        // pre/post decrement
        --it;
        TEST_ASSERT(*it == &a);
        TEST_ASSERT(it == l.begin());
        it = l.end();
        it--;
        TEST_ASSERT(*it == &c);
        // const_Iterator over a const reference
        {
            const SimpleList &cl = l;
            SimpleList::const_iterator ci;

            n = 0;
            for (ci = cl.begin(); ci != cl.end(); ++ci)
                n++;
            TEST_ASSERT(n == 3);
            ci = cl.begin();
            TEST_ASSERT(*ci == &a);
        }
        pass("test_simple_list_iter");
    }
};

// 17: SimpleAutoPtr (MINIMUM_SET: the pseudo-template local-class macro)
class AutoPtrTest : public TestCase {
  public:
    AutoPtrTest() : TestCase("test_auto_ptr") {}
    virtual ~AutoPtrTest() {}
    virtual void runTest()
    {
        Probe *raw;

        // scope-end delete, through the VIRTUAL dtor of a derived
        g_probe_dtor = 0;
        g_derived_dtor = 0;
        {
            cu_AUTO_PTR(Probe) p((Probe *)new DerivedProbe());
            TEST_ASSERT(p.get() != 0);
        }
        TEST_ASSERT(g_probe_dtor == 1);
        TEST_ASSERT(g_derived_dtor == 1);

        // release: ownership leaves the pointer, no delete at scope end
        g_probe_dtor = 0;
        raw = 0;
        {
            cu_AUTO_PTR(Probe) p(new Probe());
            raw = p.release();
            TEST_ASSERT(p.get() == 0);
        }
        TEST_ASSERT(g_probe_dtor == 0);
        delete raw;
        TEST_ASSERT(g_probe_dtor == 1);

        // reset deletes the old object; empty ptr = NULL delete no-op
        g_probe_dtor = 0;
        {
            cu_AUTO_PTR(Probe) p(new Probe());
            p.reset(new Probe());
            TEST_ASSERT(g_probe_dtor == 1);
        }
        TEST_ASSERT(g_probe_dtor == 2);
        {
            cu_AUTO_PTR(Probe) p;
            TEST_ASSERT(p.get() == 0);
        }
        pass("test_auto_ptr");
    }
};

// ------------------------------------------------------------------
// driver
// ------------------------------------------------------------------

int main()
{
    TestResult result;
    TestSuite suite("all_test_tpp");
    int n;

    suite.addTest(new CaseLifecycleTest());
    suite.addTest(new CaseFailureRecordTest());
    suite.addTest(new AssertEqualsTest());
    suite.addTest(new ResultCountsTest());
    suite.addTest(new FailureDetailTest());
    suite.addTest(new SuiteRunTest());
    suite.addTest(new RegistryTest());
    suite.addTest(new RunnerRunTest());
    suite.addTest(new RepeatedRunTest());
    suite.addTest(new DecoratorTest());
    suite.addTest(new SetupHooksTest());
    suite.addTest(new ListenerTest());
    suite.addTest(new SimpleStringTest());
    suite.addTest(new SimpleStringGrowTest());
    suite.addTest(new SimpleListTest());
    suite.addTest(new SimpleListIterTest());
    suite.addTest(new AutoPtrTest());

    suite.run(&result);

    n = result.runCount();
    printf("TESTS:%d\n", n);
    printf("FAILURES:%d\n", result.failureCount());
    printf("ERRORS:%d\n", result.errorCount());
    if (n == 17 && result.wasSuccessful())
        return 0;
    return 1;
}

