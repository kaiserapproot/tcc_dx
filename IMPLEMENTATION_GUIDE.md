# TCC C++98 実装詳細ガイド

**対象**：PLAN.md の各フェーズにおける実装の詳細化資料  
**参照**：PLAN.md の各セクションから参照される補足情報

---

## 1. libcunit.a 改造ポイント（Stage 別）

### Stage 1・2
改造不要。libcunit は純粋 C コード。

### Stage 3 改造例：テスト関数のメンバ関数化

**変更前**（test_cunit.c）
```c
static void test_func_basic(void) {
    int result = add_numbers(2, 3);
    CU_ASSERT_EQUAL(result, 5);
}

void register_tests(void) {
    CU_add_test(pSuite, "add_numbers", test_func_basic);
}
```

**変更後**（Stage 3 対応）
```c
class TestSuite {
private:
    const char *name;
public:
    TestSuite(const char *n) { name = n; }
    
    void test_func_basic(void) {
        int result = add_numbers(2, 3);
        CU_ASSERT_EQUAL(result, 5);
    }
};

void register_tests(void) {
    TestSuite suite("BasicTests");
    suite.test_func_basic();  // メンバ関数呼び出し
}
```

### Stage 4 改造例：継承ベースのテストノード

**変更前**
```c
struct TestNode {
    const char *name;
    void (*run_func)(void);
};

TestNode nodes[] = {
    {"test1", test_func_1},
    {"test2", test_func_2},
    {NULL, NULL}
};

void run_all_tests(void) {
    for (int i = 0; nodes[i].name; i++) {
        nodes[i].run_func();
    }
}
```

**変更後**（Stage 4 対応）
```c
class TestNode {
protected:
    const char *name;
public:
    TestNode(const char *n) : name(n) {}
    virtual void run() = 0;
    const char *get_name() { return name; }
};

class BasicTests : public TestNode {
public:
    BasicTests() : TestNode("BasicTests") {}
    void run() {
        test_func_basic();
    }
};

class AdvancedTests : public TestNode {
public:
    AdvancedTests() : TestNode("AdvancedTests") {}
    void run() {
        test_func_advanced();
    }
};

void run_all_tests(void) {
    TestNode *tests[] = {
        new BasicTests(),
        new AdvancedTests(),
        NULL
    };
    
    for (int i = 0; tests[i]; i++) {
        printf("Running %s\n", tests[i]->get_name());
        tests[i]->run();
    }
}
```

### Stage 5 改造例：メンバ関数ポインタによるコールバック

**変更後**（メンバポインタ対応）
```c
class TestRunner {
private:
    TestNode *nodes[10];
    int count;
public:
    TestRunner() { count = 0; }
    
    void add_test(TestNode *node) {
        nodes[count++] = node;
    }
    
    void execute_all(void (TestNode::*callback)()) {
        for (int i = 0; i < count; i++) {
            (nodes[i]->*callback)();  // メンバ関数ポインタ呼び出し
        }
    }
};

// 使用例
TestRunner runner;
runner.add_test(new BasicTests());
runner.add_test(new AdvancedTests());

void (TestNode::*pmf)() = &TestNode::run;
runner.execute_all(pmf);
```

---

## 2. TCC ビルドスクリプト完全版

### make_lib_cpp_stage1.bat

```batch
@echo off
setlocal
REM Stage1 版 libcunit 再ビルド（TCC 用）
REM 変更なし：既存の make_lib.bat と同一

cd /d "%~dp0"

set TCC_ROOT=E:\work\work_github\tpp\tcc_dx\dev\
set TCC=%TCC_ROOT%tcc.exe

echo === Stage1: Compiling CUnit (pure C) ===

%TCC% -c -m64 Automated.c -o Automated.o
if errorlevel 1 (echo ERROR: Automated.c & exit /b 1)

%TCC% -c -m64 Basic.c -o Basic.o
if errorlevel 1 (echo ERROR: Basic.c & exit /b 1)

%TCC% -c -m64 Console.c -o Console.o
if errorlevel 1 (echo ERROR: Console.c & exit /b 1)

%TCC% -c -m64 CUError.c -o CUError.o
if errorlevel 1 (echo ERROR: CUError.c & exit /b 1)

%TCC% -c -m64 MyMem.c -o MyMem.o
if errorlevel 1 (echo ERROR: MyMem.c & exit /b 1)

%TCC% -c -m64 TestDB.c -o TestDB.o
if errorlevel 1 (echo ERROR: TestDB.c & exit /b 1)

%TCC% -c -m64 TestRun.c -o TestRun.o
if errorlevel 1 (echo ERROR: TestRun.c & exit /b 1)

%TCC% -c -m64 test_cunit.c -o test_cunit.o
if errorlevel 1 (echo ERROR: test_cunit.c & exit /b 1)

%TCC% -c -m64 Util.c -o Util.o
if errorlevel 1 (echo ERROR: Util.c & exit /b 1)

%TCC% -c -m64 Win.c -o Win.o
if errorlevel 1 (echo ERROR: Win.c & exit /b 1)

echo === Creating archive ===
%TCC% -ar rcs libcunit_cpp_stage1.a Automated.o Basic.o Console.o CUError.o ^
    MyMem.o TestDB.o TestRun.o test_cunit.o Util.o Win.o
if errorlevel 1 (echo ERROR: Archive creation failed & exit /b 1)

echo === Success ===
echo libcunit_cpp_stage1.a generated successfully
dir libcunit_cpp_stage1.a
pause
```

### make_lib_cpp_stage3.bat

```batch
@echo off
setlocal
REM Stage3 版 libcunit 再ビルド（メンバ関数対応、TCC 用）
cd /d "%~dp0"

set TCC_ROOT=E:\work\work_github\tpp\tcc_dx\dev\
set TCC=%TCC_ROOT%tcc.exe

echo === Stage3: Compiling CUnit (with member functions) ===
echo NOTE: test_cunit.c has been modified to use member functions

%TCC% -c -m64 Automated.c -o Automated.o
if errorlevel 1 (echo ERROR: Automated.c & exit /b 1)

%TCC% -c -m64 Basic.c -o Basic.o
%TCC% -c -m64 Console.c -o Console.o
%TCC% -c -m64 CUError.c -o CUError.o
%TCC% -c -m64 MyMem.c -o MyMem.o
%TCC% -c -m64 TestDB.c -o TestDB.o
%TCC% -c -m64 TestRun.c -o TestRun.o

REM test_cunit.c がメンバ関数版に改造されている
%TCC% -c -m64 test_cunit.c -o test_cunit.o
if errorlevel 1 (echo ERROR: test_cunit.c (member function version) & exit /b 1)

%TCC% -c -m64 Util.c -o Util.o
%TCC% -c -m64 Win.c -o Win.o

echo === Creating archive ===
%TCC% -ar rcs libcunit_cpp_stage3.a Automated.o Basic.o Console.o CUError.o ^
    MyMem.o TestDB.o TestRun.o test_cunit.o Util.o Win.o
if errorlevel 1 (echo ERROR: Archive creation failed & exit /b 1)

echo === Success ===
echo libcunit_cpp_stage3.a generated successfully
dir libcunit_cpp_stage3.a
pause
```

### make_lib_cpp_final.bat

```batch
@echo off
setlocal
REM Final 版 libcunit 再ビルド（メンバポインタ対応、TCC 用）
cd /d "%~dp0"

set TCC_ROOT=E:\work\work_github\tpp\tcc_dx\dev\
set TCC=%TCC_ROOT%tcc.exe

echo === Final: Compiling CUnit (with member pointers) ===
echo NOTE: test_cunit.c and TestRunner have been modified for member pointers

%TCC% -c -m64 Automated.c -o Automated.o
%TCC% -c -m64 Basic.c -o Basic.o
%TCC% -c -m64 Console.c -o Console.o
%TCC% -c -m64 CUError.c -o CUError.o
%TCC% -c -m64 MyMem.c -o MyMem.o
%TCC% -c -m64 TestDB.c -o TestDB.o
%TCC% -c -m64 TestRun.c -o TestRun.o

REM test_cunit.c と TestRunner がメンバポインタ対応版
%TCC% -c -m64 test_cunit.c -o test_cunit.o
if errorlevel 1 (echo ERROR: test_cunit.c (member pointer version) & exit /b 1)

%TCC% -c -m64 Util.c -o Util.o
%TCC% -c -m64 Win.c -o Win.o

echo === Creating archive ===
%TCC% -ar rcs libcunit_cpp_final.a Automated.o Basic.o Console.o CUError.o ^
    MyMem.o TestDB.o TestRun.o test_cunit.o Util.o Win.o
if errorlevel 1 (echo ERROR: Archive creation failed & exit /b 1)

echo === Success ===
echo libcunit_cpp_final.a generated successfully
echo.
echo All stages compiled successfully!
dir libcunit_cpp_final.a
pause
```

---

## 3. ABI 別 `this` レジスタ配置のコード例

### x86_64（Windows x64、RCX にセット）

**C コード例**
```c
class Point {
    int x, y;
public:
    void set(int a, int b) { x = a; y = b; }
};

int main() {
    Point p;
    p.set(10, 20);  // Point::set(&p, 10, 20) として実装
}
```

**x86_64 アセンブリ出力例**
```asm
; メンバ関数呼び出し前処理
mov rax, [rbp - 8]      ; p のアドレス（this ポインタ）を RAX に
mov rcx, rax            ; RAX を RCX に移動（第1引数 = this）
mov edx, 10             ; EDX = x（第2引数、32ビット）
mov r8d, 20             ; R8D = y（第3引数、32ビット）
call Point::set         ; メンバ関数呼び出し

; Point::set のプロローグ
Point::set:
  push rbp
  mov rbp, rsp
  ; RCX = this pointer
  ; RDX = a (arg1)
  ; R8D = b (arg2)
  
  ; メンバ変数アクセス例：this->x = a
  mov [rcx], edx        ; this->x に a を格納
  mov [rcx + 4], r8d    ; this->y に b を格納
  
; Point::set のエピローグ
  pop rbp
  ret
```

**x86_64-gen.c での実装フック**
```c
// この部分を tccgen.c の gfunc_call に追加
if (is_member_function(s)) {
    // this を RCX に配置
    gen_move_to_register(TREG_RCX, vtop);  // RCX = this
    
    // 他の引数を RSI, RDX, R8, R9 に配置
    // 例：
    gen_move_to_register(TREG_RDX, vtop - 1);  // RDX = arg1
    gen_move_to_register(TREG_R8, vtop - 2);   // R8 = arg2
}
```

---

### ARM 32-bit（AAPCS、R0 にセット）

**ARM 32-bit アセンブリ出力例**
```asm
; メンバ関数呼び出し前処理
ldr r0, [sp]            ; R0 = this pointer（第1引数）
mov r1, #10             ; R1 = x（第2引数）
mov r2, #20             ; R2 = y（第3引数）
bl Point::set           ; メンバ関数呼び出し（Branch with Link）

; Point::set のプロローグ（AAPCS）
Point::set:
  stmfd sp!, {r7, lr}   ; フレームポインタとリターンアドレスを保存
  add r7, sp, #4        ; フレームポインタを設定
  sub sp, sp, #8        ; ローカル変数用スタック確保
  ; R0 = this pointer
  ; R1 = a (arg1)
  ; R2 = b (arg2)

  ; メンバ変数アクセス例
  str r1, [r0]          ; this->x に a を格納
  str r2, [r0, #4]      ; this->y に b を格納
  
; Point::set のエピローグ
  sub sp, r7, #4
  ldmfd sp!, {r7, lr}
  bx lr                 ; リターン
```

**arm-gen.c での実装フック**
```c
// AAPCS: R0, R1, R2, R3 に引数を順に配置
if (is_member_function(s)) {
    gen_move_to_register(0, vtop);       // R0 = this
    gen_move_to_register(1, vtop - 1);   // R1 = arg1
    gen_move_to_register(2, vtop - 2);   // R2 = arg2
    gen_move_to_register(3, vtop - 3);   // R3 = arg3
    // 4個以上の引数はスタックに PUSH
}
```

---

### ARM 64-bit / AArch64（AAPCS64、X0 にセット、16 バイト境界）

**ARM 64-bit アセンブリ出力例**
```asm
; メンバ関数呼び出し前処理
ldr x0, [sp]            ; X0 = this pointer（第1引数）
mov w1, #10             ; W1 = x（第2引数、32ビット）
mov w2, #20             ; W2 = y（第3引数、32ビット）
bl Point::set           ; メンバ関数呼び出し

; Point::set のプロローグ（AAPCS64、16 バイト境界）
Point::set:
  stp x29, x30, [sp, #-16]!  ; フレームポインタとリターンアドレスを保存（SP を 16 bytes 下げる）
  mov x29, sp                ; フレームポインタを設定
  ; X0 = this pointer
  ; W1 = a (arg1, 32ビット)
  ; W2 = b (arg2, 32ビット)

  ; メンバ変数アクセス例
  str w1, [x0]          ; this->x に a を格納（32ビット）
  str w2, [x0, #4]      ; this->y に b を格納（32ビット）
  
; Point::set のエピローグ（16 バイト境界復帰）
  ldp x29, x30, [sp], #16    ; フレームポインタとリターンアドレスを復帰、SP を 16 bytes 上げる
  ret                        ; リターン
```

**arm64-gen.c での実装フック**
```c
// AAPCS64: X0-X7 に引数を順に配置、スタックは 16 バイト境界
if (is_member_function(s)) {
    gen_move_to_register(0, vtop);       // X0 = this
    gen_move_to_register(1, vtop - 1);   // X1 = arg1
    gen_move_to_register(2, vtop - 2);   // X2 = arg2
    gen_move_to_register(3, vtop - 3);   // X3 = arg3
    gen_move_to_register(4, vtop - 4);   // X4 = arg4
    gen_move_to_register(5, vtop - 5);   // X5 = arg5
    gen_move_to_register(6, vtop - 6);   // X6 = arg6
    gen_move_to_register(7, vtop - 7);   // X7 = arg7
    
    // 8個以上の引数はスタックに配置（16 バイト境界を維持）
    align_stack_to_16();  // SP % 16 == 0 を確認
}
```

---

### 各アーキテクチャの呼び出し規約対比

| 項目 | x86_64 | ARM 32-bit | ARM 64-bit |
| :--- | :--- | :--- | :--- |
| **this ポインタ** | RCX | R0 | X0 |
| **引数1** | RDX | R1 | X1 |
| **引数2** | R8 | R2 | X2 |
| **引数3** | R9 | R3 | X3 |
| **スタック調整** | 8 バイト（AMD64） | 4 バイト（AAPCS） | **16 バイト** ⚠️ |
| **フレームポインタ** | RBP | R7 | X29 |
| **リターンレジスタ** | RAX | R0 | X0 |

**注記**：
- x86_64: RBP - RSP % 16 == 0 at function entry（caller による調整）
- ARM 32-bit: スタック 4 バイト境界（簡単）
- ARM 64-bit: スタック 16 バイト境界（macOS ではさらに厳格）

---

### TCC コード生成での注意点

**型チェック関数**
```c
int is_member_function(Sym *s) {
    // シンボル s がメンバ関数かどうかを判定
    // Sym->parent_class が NULL でない → メンバ関数
    return s && s->parent_class != NULL;
}
```

**レジスタ配置ヘルパー**
```c
void gen_move_to_register(int target_reg, SValue *src) {
    // src の値を target_reg に移動
    // プラットフォーム別に gen_mov_reg() 等を呼び出し
    
    if (target_reg == TREG_RCX) {
        gen_mov_reg(target_reg, src);  // x86_64
    } else if (target_reg == 0) {
        gen_mov_reg(0, src);            // ARM 32/64 (R0/X0)
    }
}
```

---

## 4. テスト出力ログテンプレート

### CUnit 出力形式（con_vs_test、MSVC ビルド）

**test_stage1_keywords.c 実行結果例**
```
================================
   CUNIT TEST RUNNER v1.0
================================

Suite: Stage1Tests
  Test: test_class_keyword_recognized
    Assertion: CU_ASSERT(sizeof(Point) == 16)
    → PASS ✓
    
  Test: test_bool_type_parse
    Assertion: CU_ASSERT_EQUAL(true_val, 1)
    → PASS ✓
    Assertion: CU_ASSERT_EQUAL(false_val, 0)
    → PASS ✓
    
  Test: test_access_control_default
    Assertion: CU_ASSERT(member_offset == sizeof_class)
    → PASS ✓

Run Summary: Type           Total    Count
Tests run                        3
Suites run                       1
Failures                         0
Errors                           0
Success Rate:                  100%

CUNIT: All tests passed.
```

**test_stage2_mangling.c 実行結果例**
```
================================
   CUNIT TEST RUNNER v1.0
================================

Suite: Stage2Tests
  Test: test_mangling_basic
    Assertion: CU_ASSERT_STRING_EQUAL(mangled, "_ZN5Point3setEii")
    Output: "_ZN5Point3setEii"
    → PASS ✓
    
  Test: test_mangling_qualified_names
    Assertion: CU_ASSERT_STRING_EQUAL(mangled, "_ZN5Point3addERKS_")
    Output: "_ZN5Point3addERKS_"
    → PASS ✓
    
  Test: test_scope_qualification
    Assertion: CU_ASSERT(scope_check == 1)
    → PASS ✓

Run Summary: Type           Total    Count
Tests run                        3
Suites run                       1
Failures                         0
Errors                           0
Success Rate:                  100%
```

### CPPUnit 出力形式（tcc_cpp_unittest、MSVC ビルド）

**cpp_test_stage1.cpp 実行結果例**
```
================================
 CPPUnit Test Results v1.0
================================

Test Suite: Stage1Tests
  Duration: 0.156 seconds

  [✓ PASS] ClassKeywordRecognized (35 ms)
    Expected: sizeof(Point) >= 8
    Actual:   16
    
  [✓ PASS] BoolTypeUsage (28 ms)
    bool true_value  = 1 (expected: 1) ✓
    bool false_value = 0 (expected: 0) ✓
    
  [✓ PASS] DefaultAccessControl (42 ms)
    Private member offset: 0 (verified)

Total:    3
Passed:   3
Failed:   0
Errors:   0
Success Rate: 100%

Status: ✓ ALL TESTS PASSED
```

**cpp_test_stage3.cpp 実行結果例**
```
================================
 CPPUnit Test Results v1.0
================================

Test Suite: Stage3Tests (Member Functions & this Binding)
  Duration: 0.312 seconds

  [✓ PASS] MemberFunctionCall (67 ms)
    Point p;
    p.set(10, 20);
    Assertion: p.x == 10 ✓
    Assertion: p.y == 20 ✓
    
  [✓ PASS] ThisPointerCorrect (54 ms)
    &p (object addr):       0x7ffd4a3a1928
    this inside set():      0x7ffd4a3a1928
    Difference:             0 (correct) ✓
    
  [✓ PASS] MemberAccessThroughThis (45 ms)
    member direct access:   Point::x = 100
    member via this:        this->x = 100
    Match: YES ✓

Total:    3
Passed:   3
Failed:   0
Errors:   0
Success Rate: 100%

Status: ✓ ALL TESTS PASSED
```

### TCC 出力形式（TCC で再コンパイル・実行）

**make_lib_cpp_stage1.bat 実行結果**
```
=== Stage1: Compiling CUnit (pure C) ===
Compiling Automated.c... OK
Compiling Basic.c... OK
Compiling Console.c... OK
Compiling CUError.c... OK
Compiling MyMem.c... OK
Compiling TestDB.c... OK
Compiling TestRun.c... OK
Compiling test_cunit.c... OK
Compiling Util.c... OK
Compiling Win.c... OK

=== Creating archive ===
libcunit_cpp_stage1.a created successfully (523 KB)

=== Success ===
libcunit_cpp_stage1.a generated successfully
Volume Serial Number is 1A2B-3C4D

Directory of E:\work\work_github\tpp\tcc_dx\dev\cunit

2026-05-23  15:42    523,456  libcunit_cpp_stage1.a
               1 File(s)    523,456 bytes
```

**make_lib_cpp_stage3.bat 実行結果**
```
=== Stage3: Compiling CUnit (with member functions) ===
NOTE: test_cunit.c has been modified to use member functions
Compiling Automated.c... OK
Compiling Basic.c... OK
Compiling Console.c... OK
Compiling CUError.c... OK
Compiling MyMem.c... OK
Compiling TestDB.c... OK
Compiling TestRun.c... OK
Compiling test_cunit.c (member function version)... OK
  → Class TestNode recognized
  → Member function test_basic() compiled
  → this binding verified
Compiling Util.c... OK
Compiling Win.c... OK

=== Creating archive ===
libcunit_cpp_stage3.a created successfully (548 KB)

=== Success ===
libcunit_cpp_stage3.a generated successfully

Directory of E:\work\work_github\tpp\tcc_dx\dev\cunit

2026-05-23  16:05    548,892  libcunit_cpp_stage3.a
               1 File(s)    548,892 bytes
```

### テキスト比較検証（MSVC 出力 ↔ TCC 出力）

**一致する場合（成功例）**
```
$ fc test_stage1_msvc_output.txt test_stage1_tcc_output.txt
Comparing files test_stage1_msvc_output.txt and test_stage1_tcc_output.txt
FC: no differences encountered

=== Comparison Result ===
Status: MATCH ✓
MSVC run:  3 tests, 3 passed, 0 failed (100%)
TCC run:   3 tests, 3 passed, 0 failed (100%)
Difference: NONE (exact match)
```

**不一致する場合（失敗例）**
```
$ fc test_stage1_msvc_output.txt test_stage1_tcc_output.txt
Comparing files test_stage1_msvc_output.txt and test_stage1_tcc_output.txt
***** test_stage1_msvc_output.txt
sizeof(Point) = 16
***** test_stage1_tcc_output.txt
sizeof(Point) = 12

=== Comparison Result ===
Status: MISMATCH ✗
Line 5: Size difference detected
MSVC: Point class size = 16 bytes
TCC:  Point class size = 12 bytes
Error: Member function offset calculation differs

Action Required:
  1. Check padding/alignment logic in tccgen.c
  2. Verify layout against MSVC expectations
  3. Rebuild with debug trace enabled
```

---

### テスト実行スクリプト例

**run_all_tests.bat**
```batch
@echo off
setlocal enabledelayedexpansion

REM ========================================
REM Stage 1-4 テスト統合実行
REM ========================================

set LOG_DIR=%TEMP%\tcc_test_logs
if not exist %LOG_DIR% mkdir %LOG_DIR%

echo === MSVC CUnit Tests ===
con_vs_test.exe > %LOG_DIR%\msvc_stage1.log 2>&1
echo MSVC Stage1: %ERRORLEVEL%

echo === MSVC CPPUnit Tests ===
tcc_cpp_unittest.exe > %LOG_DIR%\msvc_cpp_stage1.log 2>&1
echo MSVC CPPUnit: %ERRORLEVEL%

echo === TCC Build ===
cd dev\cunit
call make_lib_cpp_final.bat > %LOG_DIR%\tcc_build.log 2>&1
echo TCC Build: %ERRORLEVEL%

echo === Output Comparison ===
fc %LOG_DIR%\msvc_stage1.log %LOG_DIR%\tcc_stage1.log
if errorlevel 1 (
    echo ERROR: Output mismatch!
    exit /b 1
) else (
    echo SUCCESS: All outputs match!
)

pause
```

---

## 5. メンバポインタ移行条件チェックリスト

### フェーズ 1：データメンバポインタ（DMP）

#### DMP 完了判定基準

**MSVC 側（con_vs_test）**

```
[ ] test_member_pointer.c コンパイル成功（エラーなし）
    - クラス定義での &Class::member 構文を認識
    - 型チェック: int Point::*px; の宣言成功

[ ] テストケース実行
    Test: test_data_member_pointer_basic
      - [ ] int Point::*px; の宣言成功
      - [ ] px = &Point::x; でアドレス取得
      - [ ] p.*px でメンバ値の読み取り
      - [ ] p.*px = value で値の書き込み
      Result: PASS
    
    Test: test_dmp_multiple_pointers
      - [ ] 複数の DMP を同時に使用可能か
      - [ ] offset 値の計算が正確か
      Result: PASS
      
    Test: test_dmp_array_of_pointers
      - [ ] DMP 配列の初期化と使用
      - [ ] ループでの間接アクセス
      Result: PASS

[ ] 出力確認例
    MSVC Output:
    ==================
    test_dmp_basic:      PASS
      p.x = 10 (via ptr) ✓
      p.y = 20 (via ptr) ✓
    
    test_dmp_multiple:   PASS
      px points to x offset +0 ✓
      py points to y offset +4 ✓
    
    test_dmp_array:      PASS
      ptrs[0] = &Point::x ✓
      ptrs[1] = &Point::y ✓
    ==================
```

**TCC 側テスト**

```
[ ] TCC で再コンパイル成功
    make_lib_cpp_stage4.bat 実行
    → libcunit_cpp_stage4.a 生成成功

[ ] TCC コンパイラ確認事項
    - [ ] tcctok.h に member_pointer トークン登録
    - [ ] tccgen.c で &Class::member 構文解析
    - [ ] オフセット計算ロジック実装
    - [ ] x86_64/ARM/ARM64 プラットフォーム対応

[ ] 実行結果確認
    TCC Output:
    ==================
    Stage4 DMP Tests:
      test_dmp_basic:      PASS
      test_dmp_multiple:   PASS
      test_dmp_array:      PASS
    ==================
```

**出力比較**

```
[ ] MSVC 出力 vs TCC 出力
    $ fc msvc_dmp_output.txt tcc_dmp_output.txt
    
    Comparison Points:
    - [ ] オフセット値の一致（例：&Point::x = 0, &Point::y = 4）
    - [ ] メモリ値の一致（p.*px の結果）
    - [ ] エラーメッセージなし（両方共）
    
    Result: NO DIFFERENCES ✓
```

---

### フェーズ 2：メンバ関数ポインタ（MFP）

#### MFP 完了判定基準

**MSVC 側（con_vs_test）**

```
[ ] test_member_pointer.c に MFP テスト追加＆コンパイル成功
    - クラスメソッド定義での &Class::method 構文を認識
    - 型チェック: void (Point::*pmf)(); の宣言成功

[ ] テストケース実行
    Test: test_member_function_pointer_basic
      - [ ] void (Point::*pmf)(); の宣言成功
      - [ ] pmf = &Point::reset; でメソッドアドレス取得
      - [ ] (p.*pmf)() でメソッドポインタ呼び出し
      - [ ] this が正しく渡されるか
      Result: PASS
    
    Test: test_mfp_invoke_with_args
      - [ ] int (Point::*pmf)(int); の宣言
      - [ ] (p.*pmf)(value) で引数付き呼び出し
      - [ ] 戻り値が期待値と一致
      Result: PASS
      
    Test: test_mfp_polymorphic_simple
      - [ ] 単一継承でのメソッドポインタ呼び出し
      - [ ] 仮想メソッドは未対応（明記）
      Result: PASS

[ ] 出力確認例
    MSVC Output:
    ==================
    test_mfp_basic:      PASS
      p.reset() via ptr   ✓ (p.x = 0, p.y = 0)
    
    test_mfp_args:       PASS
      int val = (p.*pmf)(10);  ✓ (val = 20)
    
    test_mfp_inherit:    PASS
      Base::method via ptr ✓
    
    NOTE: Virtual methods not yet supported
    ==================
```

**TCC 側テスト**

```
[ ] TCC で再コンパイル成功
    make_lib_cpp_final.bat 実行
    → libcunit_cpp_final.a 生成成功

[ ] TCC コンパイラ確認事項
    - [ ] MFP 型の解析（tccgen.c に type_mfp 処理）
    - [ ] (obj.*pmf)() 構文解析
    - [ ] this 自動挿入ロジック実装
    - [ ] 単一継承下での offset 計算
    - [ ] 各プラットフォーム (x86_64/ARM/ARM64) 対応

[ ] 実行結果確認
    TCC Output:
    ==================
    Stage5 MFP Tests:
      test_mfp_basic:      PASS
      test_mfp_args:       PASS
      test_mfp_inherit:    PASS
      test_mfp_virtual:    SKIP (not yet supported)
    ==================
```

**出力比較**

```
[ ] MSVC 出力 vs TCC 出力
    $ fc msvc_mfp_output.txt tcc_mfp_output.txt
    
    Comparison Points:
    - [ ] メソッド呼び出し結果の一致
    - [ ] 戻り値の一致
    - [ ] 状態変更の一致（p.x, p.y 値）
    - [ ] エラーメッセージなし（両方共）
    - [ ] Virtual method SKIP 表示が同一
    
    Result: NO DIFFERENCES ✓
```

---

### Phase 1 → Phase 2 移行ゲート

```
前提条件：
  ✓ Phase 1 (DMP) すべてのテスト PASS
  ✓ MSVC CUnit/CPPUnit すべてのテスト PASS
  ✓ TCC でのビルド成功、出力が MSVC と一致

移行ステップ：

1. Code Review（実装品質確認）
   [ ] DMP 実装の完成度を確認
       - tcctok.h: member_pointer トークン登録
       - tccgen.c: &Class::member 解析ロジック
       - x86_64/arm/arm64-gen.c: プラットフォーム対応
   
   [ ] ドキュメンテーション
       - [ ] DMP の制限事項を IMPLEMENTATION_GUIDE.md に記載
       - [ ] 複数継承未対応を明記
       - [ ] オフセット計算の方式を記述

2. リグレッション確認
   [ ] Stage 1-4 全テストが依然として PASS か確認
       $ con_vs_test.exe  → 全テスト PASS
       $ tcc_cpp_unittest.exe → 全テスト PASS
   
   [ ] libcunit.a の再ビルドが成功するか確認
       $ make_lib_cpp_final.bat → OK

3. MFP 実装前の準備
   [ ] tccgen.c の MFP パーサ構造を設計
   [ ] (obj.*pmf)() 構文の解析フロー図作成
   [ ] プラットフォーム別 this 挿入ロジックを確認

4. 正式な Phase 2 開始通知
   [ ] チェックリスト以下が全て完了
   [ ] Phase 2 MFP テスト設計ドキュメント準備完了
```

---

### 統合テスト（Phase 1 + Phase 2）

#### すべてのテストを一括実行

```batch
@echo off
REM run_final_integration_test.bat

echo === Final Integration Test ===
echo Testing: Stage1-4 + DMP (Phase1) + MFP (Phase2)
echo.

REM MSVC CUnit
echo Step1: MSVC CUnit Tests
con_vs_test.exe > final_msvc_cunit.log
if errorlevel 1 goto fail_cunit

REM MSVC CPPUnit
echo Step2: MSVC CPPUnit Tests
tcc_cpp_unittest.exe > final_msvc_cpp.log
if errorlevel 1 goto fail_cpp

REM TCC Build Stage1-4
echo Step3: TCC Build (Stage1-4)
cd dev\cunit
call make_lib_cpp_stage4.bat > ..\final_tcc_build_stage4.log
if errorlevel 1 goto fail_tcc_build
cd ..\..

REM TCC Build MFP
echo Step4: TCC Build (with MFP)
cd dev\cunit
call make_lib_cpp_final.bat > ..\final_tcc_build_final.log
if errorlevel 1 goto fail_tcc_final
cd ..\..

REM Output Comparison
echo Step5: Output Comparison
fc final_msvc_cunit.log final_tcc_cunit.log
if errorlevel 1 goto fail_compare

echo.
echo === ALL TESTS PASSED ===
echo ✓ Stage1-4 implemented and tested
echo ✓ DMP (Phase1) working correctly
echo ✓ MFP (Phase2) working correctly
echo ✓ MSVC and TCC outputs match exactly
echo.
goto end

:fail_cunit
echo ERROR: MSVC CUnit test failed
goto end

:fail_cpp
echo ERROR: MSVC CPPUnit test failed
goto end

:fail_tcc_build
echo ERROR: TCC build (Stage1-4) failed
goto end

:fail_tcc_final
echo ERROR: TCC build (with MFP) failed
goto end

:fail_compare
echo ERROR: Output comparison failed - MSVC vs TCC mismatch
goto end

:end
pause
```

---

## 参考：実装手順チェックシート

| # | 項目 | 状態 | 備考 |
| :---: | :--- | :---: | :--- |
| 1 | libcunit改造例理解 | □ | このドキュメント参照 |
| 2 | make_lib_cpp_*.bat 作成 | □ | このドキュメントのスクリプトをコピー |
| 3 | ABI コード例確認（詳細版） | □ | アセンブリコード例付き |
| 4 | テスト出力形式理解（詳細版） | □ | 各ステージのログテンプレート |
| 5 | メンバポインタ移行基準理解 | □ | DMP/MFP 統合チェックリスト |
| 6 | 統合テスト実行 | □ | run_final_integration_test.bat |
| 7 | 実装開始 | □ | PLAN.md Phase 1 より開始 |

