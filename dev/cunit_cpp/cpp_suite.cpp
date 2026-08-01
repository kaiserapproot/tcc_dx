/* libcunit を C++ の機能で駆動するテストドライバ
 *
 * PLAN.md の Stage 4/5 に対応する改造例:
 *   Stage 4 … テストスイートを継承ベースで表現し、仮想関数で振る舞いを差し替える
 *   Stage 5 … `void (*pFunc)(void)` を `void (TestNode::*pFunc)(void)` へ置き換える
 *
 * CUnit 本体（C API）はそのまま使い、テスト側だけを C++ 化している。
 */
#include <stdio.h>
#include <string.h>
#include "CUnit.h"
#include "Basic.h"

/* ---- Stage 4: 継承と仮想関数によるテストノード ------------------------ */

class TestNode {
public:
    int runs;
    int value;

    TestNode() : runs(0), value(0) {}
    virtual ~TestNode() {}

    virtual const char* name() = 0;      /* 純粋仮想 = 抽象インタフェース */
    virtual int compute() = 0;

    /* 非仮想メソッドから仮想メソッドを呼ぶ（動的ディスパッチ） */
    void run() {
        runs = runs + 1;
        value = compute();
    }

    /* 非仮想のラッパ。メンバ関数ポインタはこちらを指す
       （呼び出された先で仮想 compute() が動的に選ばれる） */
    int evaluate() { return compute(); }
};

class AddNode : public TestNode {
public:
    int a, b;
    AddNode(int x, int y) : a(x), b(y) {}
    const char* name() { return "add"; }
    int compute() { return a + b; }
};

class MulNode : public AddNode {          /* 3 段継承 */
public:
    MulNode(int x, int y) : AddNode(x, y) {}
    const char* name() { return "mul"; }
    int compute() { return a * b; }
};

/* ---- Stage 5: メンバ関数ポインタ -------------------------------------- */

typedef int (TestNode::* NodeFn)();

static int invoke(TestNode* n, NodeFn f)
{
    return (n->*f)();                      /* メンバ関数ポインタ経由の呼び出し */
}

/* ---- CUnit のテストケース ---------------------------------------------
   注: 静的記憶域のオブジェクトはコンストラクタが呼ばれない（PE の起動
   コードが .init_array を実行しないため）。テスト対象はすべてローカル
   またはヒープ上に作る。 */

static void test_inheritance(void)
{
    AddNode add(10, 20);
    MulNode mul(10, 20);

    CU_ASSERT_EQUAL(add.compute(), 30);
    CU_ASSERT_EQUAL(mul.compute(), 200);
    CU_ASSERT_STRING_EQUAL(add.name(), "add");
    CU_ASSERT_STRING_EQUAL(mul.name(), "mul");
}

static void test_virtual_dispatch(void)
{
    AddNode add(2, 5);
    MulNode mul(2, 5);
    TestNode* list[2];
    int i, total = 0;

    list[0] = (TestNode*)&add;
    list[1] = (TestNode*)&mul;

    for (i = 0; i < 2; i++) {
        list[i]->run();                    /* run() -> 仮想 compute() */
        total = total + list[i]->value;
    }
    CU_ASSERT_EQUAL(total, 17);            /* 7 + 10 */
    CU_ASSERT_EQUAL(list[0]->runs, 1);
    CU_ASSERT_STRING_EQUAL(list[1]->name(), "mul");
}

static void test_member_pointer(void)
{
    AddNode add(6, 7);
    MulNode mul(6, 7);
    /* 非仮想ラッパのメンバ関数ポインタ。中の compute() は動的に解決される */
    NodeFn fn = &TestNode::evaluate;
    int TestNode::* pruns = &TestNode::runs; /* データメンバポインタ */

    CU_ASSERT_EQUAL(invoke((TestNode*)&add, fn), 13);
    CU_ASSERT_EQUAL(invoke((TestNode*)&mul, fn), 42);

    add.run();
    add.run();
    CU_ASSERT_EQUAL(add.*pruns, 2);
    add.*pruns = 9;
    CU_ASSERT_EQUAL(add.runs, 9);
}

static void test_heap_objects(void)
{
    TestNode* p = (TestNode*)new MulNode(5, 5);
    CU_ASSERT_EQUAL(p->compute(), 25);
    CU_ASSERT_STRING_EQUAL(p->name(), "mul");
    p->run();
    CU_ASSERT_EQUAL(p->value, 25);
    delete p;                              /* 仮想デストラクタ経由 */
}

static void test_references(void)
{
    AddNode add(1, 2);
    int& r = add.a;
    r = 100;
    CU_ASSERT_EQUAL(add.a, 100);
    CU_ASSERT_EQUAL(add.compute(), 102);
}

/* ---- 関数オーバーロード ------------------------------------------------ */

static int scale(int v) { return v * 2; }
static int scale(double v) { return (int)(v * 10.0); }
static int scale(int v, int k) { return v * k; }

static void test_overload(void)
{
    CU_ASSERT_EQUAL(scale(5), 10);
    CU_ASSERT_EQUAL(scale(1.5), 15);
    CU_ASSERT_EQUAL(scale(5, 3), 15);
}

static CU_TestInfo tests_cpp[] = {
    { (char*)"inheritance",      test_inheritance },
    { (char*)"virtual dispatch", test_virtual_dispatch },
    { (char*)"member pointer",   test_member_pointer },
    { (char*)"heap new/delete",  test_heap_objects },
    { (char*)"reference",        test_references },
    { (char*)"overload",         test_overload },
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites[] = {
    { (char*)"C++ features", NULL, NULL, tests_cpp },
    CU_SUITE_INFO_NULL,
};

void* tinyc_getbp(void) { return NULL; }

int main(void)
{
    CU_ErrorCode err;
    unsigned int failed;

    if (CUE_SUCCESS != CU_initialize_registry())
        return 1;
    err = CU_register_suites(suites);
    if (CUE_SUCCESS != err) {
        printf("register failed: %d\n", (int)err);
        CU_cleanup_registry();
        return 1;
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    failed = CU_get_number_of_tests_failed();
    CU_cleanup_registry();
    printf("\nRESULT: %s\n", failed == 0 ? "ALL PASS" : "FAILED");
    return failed == 0 ? 0 : 1;
}
