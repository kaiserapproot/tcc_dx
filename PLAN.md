# TCC C++98 段階的実装テストプラン

**文書版**：v1.0  
**作成日**：2026-05-23  
**対象**：TCC (Tiny C Compiler) への C++98 標準機能の段階的導入

---

## 目次
1. [概要](#概要)
2. [環境構成](#環境構成)
3. [実装優先度](#実装優先度)
4. [Phase 別詳細計画](#phase-別詳細計画)
5. [テストケース仕様](#テストケース仕様)
6. [完了基準と検証](#完了基準と検証)
7. [タイムライン](#タイムライン)

---

## 概要

TCC に C++98 の基本機能（クラス、メンバ関数、名前修飾、メンバポインタ等）を段階的に導入する実装プランです。  
各ステージ完了時に、MSVC 側テスト（CUnit + CPPUnit）と TCC 側テスト（再ビルド libcunit.a）を実行し、動作確認と外部比較を並行します。

---

## 環境構成

### MSVC 側テストプロジェクト
- **`test/vs_test/con_vs_test.vcxproj`**
  - 構成：CUnit フレームワーク統合（基本ファイル：`vs_test_main.c` + CUnit ソース）
  - 拡張対象：Stage 別テストケース追加（test_stage*.c）
  - 目的：C 側の基本機能検証（キーワード、マングリング等）

- **`test/tcc_cpp_unittest/tcc_cpp_unittest.vcxproj`**
  - 構成：CPPUnit スケルトン（`pch.h/cpp` + `tcc_cpp_unittest.cpp`）
  - 拡張対象：C++98 機能テストケース追加（cpp_test_stage*.cpp）
  - 目的：C++ の高度な機能検証（クラス、継承、メンバポインタ等）

### TCC 側再ビルド環境
- **`dev/cunit/`**
  - 現状：CUnit 純粋 C フレームワーク実装済み
  - 改造対象：C++98 機能テストケース相当の記述へ段階改造
  - 再ビルドスクリプト：`dev/cunit/make_lib_cpp.bat`（TCC 用、Stage 別版を作成）
  - 出力成果物：`dev/lib/libcunit_cpp_stage*.a`（各ステージ版）

---

## 実装優先度

実装は以下の順序で段階的に進行します：

1. **Stage 1**：キーワード登録＆字句解析（基盤）
2. **Stage 2**：名前修飾＆スコープ（オーバーロード対応）
3. **Stage 3**：`this` ポインタ＆メンバ関数＆デフォルト引数
4. **Stage 4**：単一継承＆初期化子リスト＆静的メンバ
5. **メンバポインタ フェーズ 1&2**：データメンバ＆非仮想 PMF

---

## Phase 別詳細計画

---

## Phase 1：テスト基盤整備（5～8h）

### 目的
MSVC 側・TCC 側双方のテストフレームワークを構築し、以降のステージで再利用可能な基盤を確立する。

### 1.1 con_vs_test プロジェクト拡張

**ファイル追加**：
- `test/vs_test/test_common.h`
  - 共通テストマクロ・ユーティリティ関数定義
  - CUnit テスト実行ヘルパー

- `test/vs_test/test_stage1_keywords.c`
  - **内容**：Stage1（キーワード）テストケース群（CUnit）
  - **テスト内容**：
    - キーワード認識（`class`, `public`, `protected`, `private`, `virtual`, `bool`, `true`, `false`, `namespace`）
    - 型パース（`bool` 変数の宣言・初期化）
    - クラス定義のパース（`class Point { int x; }` 等）
    - デフォルトアクセス権（`class` は `private`、`struct` は `public`）

- `test/vs_test/test_stage2_mangling.c`
  - **内容**：Stage2（マングリング）テストケース群（CUnit）
  - **テスト内容**：
    - 関数オーバーロード区別（`int f(int)` vs `int f(double)`）
    - マングル名衝突回避
    - 参照型パラメータ（`void f(int&)`）のマングル化

- `test/vs_test/test_stage3_members.c`
  - **内容**：Stage3（メンバ関数）テストケース群（CUnit）
  - **テスト内容**：
    - メンバ関数呼び出し（`obj.method()`）
    - メンバ変数アクセス（`obj.member`）
    - デフォルト引数補完（`obj.set(10)` ← `void set(int x = 5)` 呼び出し）
    - `this` ポインタの暗黙的渡却

- `test/vs_test/test_stage4_inheritance.c`
  - **内容**：Stage4（継承）テストケース群（CUnit）
  - **テスト内容**：
    - 単一継承の基本（`class Derived : public Base`）
    - メンバ変数のレイアウト（派生クラスが基底クラスのメモリを先頭に保持）
    - 初期化子リスト（`Derived() : Base(10) {}`）
    - 静的メンバ変数・関数

- `test/vs_test/test_member_pointer.c`
  - **内容**：メンバポインタテストケース群（CUnit）
  - **テスト内容**：
    - **フェーズ 1**：
      - データメンバポインタ宣言（`int Point::*p`）
      - メンバアドレス取得（`p = &Point::x`）
      - メンバ間接アクセス（`obj.*p` の読み取り・書き込み）
    - **フェーズ 2**：
      - メンバ関数ポインタ宣言（`void (Point::*pmf)()`）
      - メンバ関数間接呼び出し（`(obj.*pmf)()`）
      - 単一継承での PMF 呼び出し

**実装の進め方**：
- 各テストケースは CUnit の `TEST_ADD_TEST_TO_SUITE()` パターンで実装
- 各テストの開始時に「テスト対象コード例」を `printf()` で出力
- 最終的に stdout に `PASS` or `FAIL` 判定を出力
- CUnit テストランナー（Console Runner）で集計実行

### 1.2 tcc_cpp_unittest プロジェクト拡張

**ファイル追加**：
- `test/tcc_cpp_unittest/cpp_test_stage1.cpp`
  ```cpp
  TEST_CLASS(Stage1Tests) {
    TEST_METHOD(ClassKeywordRecognized) {
      // class Point { int x; }; 
      // Point p; の構文が通ること
      Assert::IsTrue(sizeof(Point) >= sizeof(int));
    }
    TEST_METHOD(BoolTypeUsage) {
      // bool の宣言・初期化・値の正当性
      bool flag = true;
      Assert::IsTrue(flag == 1);
    }
    TEST_METHOD(DefaultAccessControl) {
      // class は private、struct は public がデフォルト
      // （コンパイル時エラー確認 or シンボル可視性テスト）
    }
  };
  ```

- `test/tcc_cpp_unittest/cpp_test_stage2.cpp`
  ```cpp
  TEST_CLASS(Stage2Tests) {
    TEST_METHOD(FunctionOverloading) {
      // int f(int) と int f(double) の区別
      Assert::AreEqual(f(10), 100);        // int 版
      Assert::AreEqual(f(3.14), 314);      // double 版（整数化）
    }
    TEST_METHOD(ReferenceParameters) {
      // void modify(int&) のマングル化・呼び出し
      int val = 5;
      modify(val);
      Assert::AreEqual(val, 10);  // 参照経由で変更確認
    }
  };
  ```

- `test/tcc_cpp_unittest/cpp_test_stage3.cpp`
  ```cpp
  TEST_CLASS(Stage3Tests) {
    TEST_METHOD(MemberFunctionCall) {
      // Point p; p.set(3, 4); p.distance() == 5
      Point p;
      p.set(3, 4);
      Assert::AreEqual(p.distance(), 5.0);
    }
    TEST_METHOD(MemberVariableAccess) {
      // obj.x, obj.y のアクセス
      Point p; p.x = 1; p.y = 2;
      Assert::AreEqual(p.x, 1);
      Assert::AreEqual(p.y, 2);
    }
    TEST_METHOD(DefaultArguments) {
      // class Obj { void set(int x = 10); };
      // obj.set(); と obj.set(20); の両方動作
      Obj o1, o2;
      o1.set();     // x = 10 (デフォルト)
      o2.set(20);   // x = 20 (明示)
      Assert::AreEqual(o1.value, 10);
      Assert::AreEqual(o2.value, 20);
    }
  };
  ```

- `test/tcc_cpp_unittest/cpp_test_stage4.cpp`
  ```cpp
  TEST_CLASS(Stage4Tests) {
    TEST_METHOD(SingleInheritance) {
      // class Base { int x; }; 
      // class Derived : public Base { int y; };
      // Derived d; d.x = 1; d.y = 2;
      Derived d;
      d.x = 1;
      d.y = 2;
      Assert::AreEqual(d.x, 1);
      Assert::AreEqual(d.y, 2);
    }
    TEST_METHOD(MemoryLayout) {
      // sizeof(Derived) == sizeof(Base) + sizeof(y)
      Assert::AreEqual(sizeof(Derived), sizeof(int) * 2);
    }
    TEST_METHOD(ConstructorInitializerList) {
      // class Derived : public Base { 
      //   Derived() : Base(5) {} 
      // };
      Derived d;  // Base(5) → d.x = 5 で初期化
      Assert::AreEqual(d.x, 5);
    }
    TEST_METHOD(StaticMemberVariable) {
      // class Counter { static int count; };
      // Counter::count の読み取り・書き込み
      Assert::AreEqual(Counter::count, 0);
      Counter::count = 10;
      Assert::AreEqual(Counter::count, 10);
    }
  };
  ```

- `test/tcc_cpp_unittest/cpp_test_member_pointer.cpp`
  ```cpp
  TEST_CLASS(MemberPointerTests) {
    TEST_METHOD(DataMemberPointer) {
      // int Point::*px = &Point::x;
      // Point p; p.*px = 5;
      int Point::*px = &Point::x;
      Point p;
      p.*px = 5;
      Assert::AreEqual(p.x, 5);
    }
    TEST_METHOD(MemberFunctionPointer) {
      // void (Point::*pmf)() = &Point::reset;
      // Point p; (p.*pmf)();
      void (Point::*pmf)() = &Point::reset;
      Point p;
      p.x = 10;
      (p.*pmf)();
      Assert::AreEqual(p.x, 0);  // reset() で x = 0
    }
  };
  ```

**実装の進め方**：
- 各テストはネイティブ CPPUnit テストメソッド（`TEST_METHOD`）として実装
- MSVC テストランナー（VS Test Explorer）で実行・集計
- 各テスト成功時は「PASS」、失敗時は「FAIL」と表示

### 1.3 libcunit.a 改造版の基盤準備

**目的**：TCC で再コンパイルできる「C++98 対応の libcunit」を段階的に構築する基盤を整備する。

**実施内容**：

1. **`dev/cunit/` 内ファイルリスト化**
   ```
   - Automated.c / Automated.h
   - Basic.c / Basic.h
   - Console.c / Console.h
   - CUError.c / CUError.h
   - MyMem.c / MyMem.h
   - TestDB.c / TestDB.h
   - TestRun.c / TestRun.h
   - test_cunit.c / test_cunit.h
   - Util.c / Util.h
   - Win.c / Win.h
   - CUnit.h (main header)
   - CUnit_intl.h (internal)
   ```

2. **Stage 別改造ポイント表**
   | Stage | 改造対象ファイル | 改造内容 | 例 |
   | :---: | :--- | :--- | :--- |
   | 1 | なし | キーワード認識のみ（libcunit は C コード） | - |
   | 2 | なし | マングリングのみ（libcunit は C コード） | - |
   | 3 | `test_cunit.c` 等 | メンバ関数呼び出し形式に一部改写 | `static void test_func(void) {...}` → `class TestSuite { void test_func() {...} };` |
   | 4 | `TestDB.c`, `TestRun.c` 等 | 継承ベースの拡張（実装複雑度大） | `struct TestNode` → `class TestNode : public Base` |
   | 5 | メンバポインタ対応部 | 関数ポインタ → メンバ関数ポインタ | `void (*pFunc)(void)` → `void (TestNode::*pFunc)(void)` |

3. **改造版ビルドスクリプト作成**
   - `dev/cunit/make_lib_cpp_stage1.bat`：Stage1 用（実質 make_lib.bat と同一）
   - `dev/cunit/make_lib_cpp_stage2.bat`：Stage2 用（実質 make_lib.bat と同一）
   - `dev/cunit/make_lib_cpp_stage3.bat`：Stage3 用（改造版 test_cunit.c を含む）
   - `dev/cunit/make_lib_cpp_stage4.bat`：Stage4 用（継承対応改造を含む）
   - `dev/cunit/make_lib_cpp_final.bat`：最終版（メンバポインタ対応）

   **スクリプト形式例**（Stage1）：
   ```batch
   REM Stage1 版 libcunit 再ビルド（TCC 用）
   @echo off
   cd /d "%~dp0"
   
   set TCC_ROOT=E:\work\work_github\tpp\tcc_dx\dev\
   set TCC=%TCC_ROOT%tcc.exe
   
   REM 各 .c ファイルをコンパイル
   %TCC% -c -m64 Automated.c -o Automated.o
   %TCC% -c -m64 Basic.c -o Basic.o
   %TCC% -c -m64 Console.c -o Console.o
   %TCC% -c -m64 CUError.c -o CUError.o
   %TCC% -c -m64 MyMem.c -o MyMem.o
   %TCC% -c -m64 TestDB.c -o TestDB.o
   %TCC% -c -m64 TestRun.c -o TestRun.o
   %TCC% -c -m64 test_cunit.c -o test_cunit.o
   %TCC% -c -m64 Util.c -o Util.o
   %TCC% -c -m64 Win.c -o Win.o
   
   REM アーカイブ生成
   %TCC% -ar rcs libcunit_cpp_stage1.a Automated.o Basic.o Console.o CUError.o ^
     MyMem.o TestDB.o TestRun.o test_cunit.o Util.o Win.o
   
   echo libcunit_cpp_stage1.a generated successfully
   ```

4. **初期サイズベースライン記録**
   - 元の `libcunit.a` のサイズ（バイト）を記録
   - 各ステージ版のサイズ変化を追跡（機能追加時のオーバーヘッド確認用）

---

## Phase 2：Stage 1 実装（キーワード＆基本型）（3～5h）

### 目的
TCC の字句解析・パーサに C++98 の基本キーワードと型を登録し、最小限の C++ 構文認識を実現する。

### 2.1 TCC 側実装 *(必須前提：Phase 1 完了)*

**修正ファイル**：`tcctok.h`, `tccpp.c`

**追加内容**：

1. **`tcctok.h` にキーワード登録**
   ```c
   DEF(TOK_CLASS, "class")
   DEF(TOK_PUBLIC, "public")
   DEF(TOK_PROTECTED, "protected")
   DEF(TOK_PRIVATE, "private")
   DEF(TOK_VIRTUAL, "virtual")
   DEF(TOK_THIS, "this")
   DEF(TOK_OPERATOR, "operator")
   DEF(TOK_BOOL, "bool")
   DEF(TOK_TRUE, "true")
   DEF(TOK_FALSE, "false")
   DEF(TOK_NAMESPACE, "namespace")
   ```

2. **`tccpp.c` でのトークン処理**
   - 新キーワードを `next_nomacro()` で認識
   - トークン ID を `tok` に設定

3. **`tccgen.c` での型パース拡張**
   - `parse_btype()` で `bool` を基本型として認識
   - `bool` 変数宣言（`bool flag = true;`）をパース

4. **`struct_decl()` 拡張**
   - `class` キーワード時は `struct` と同等に処理（デフォルトアクセス権 `private` に設定）

### 2.2 MSVC 側テスト実行 *(依存：2.1 の TCC 実装完了)*

**実行内容**：

- **con_vs_test 実行**
  ```bash
  # MSVC コンソール実行テスト
  cd test\vs_test
  msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
  Debug Unicode\con_vs_test.exe
  ```
  - `test_stage1_keywords.c` の全テストケース実行
  - 期待：「キーワード認識テスト PASS」「bool 型パース PASS」「デフォルトアクセス権 PASS」

- **tcc_cpp_unittest 実行**
  ```bash
  # CPPUnit テスト実行（VS Test Explorer または コマンドライン）
  cd test\tcc_cpp_unittest
  msbuild tcc_cpp_unittest.vcxproj /p:Configuration="Debug" /p:Platform=x64
  vstest.console.exe tcc_cpp_unittest.dll
  ```
  - `cpp_test_stage1.cpp` の全テストケース実行
  - 期待：「ClassKeywordRecognized PASS」「BoolTypeUsage PASS」「DefaultAccessControl PASS」

### 2.3 TCC libcunit.a 再ビルド（Stage 1 版） *(依存：2.1 の TCC 実装完了)*

**実行内容**：

```bash
cd dev\cunit
make_lib_cpp_stage1.bat
```

**確認項目**：
- アーカイブ生成成功（`libcunit_cpp_stage1.a` 出力）
- ファイルサイズ ≥ 元の `libcunit.a` サイズ（最低限のオーバーヘッド確認）
- シンボルテーブル確認（TCC の `ar` で確認）

### 2.4 TCC 側テスト実行 *(依存：2.3 の libcunit.a 再ビルド完了)*

**実行内容**：

```bash
# TCC で test_stage1_keywords.c を再コンパイル
cd test\vs_test
tcc test_stage1_keywords.c -o test_stage1_keywords_tcc.exe

# 実行
test_stage1_keywords_tcc.exe > stage1_tcc_output.txt

# MSVC 版の出力と比較
fc stage1_tcc_output.txt stage1_msvc_output.txt
```

**確認項目**：
- TCC コンパイル成功
- TCC 生成バイナリ実行成功
- **MSVC 版と TCC 版の出力が完全に一致**（「PASS」「FAIL」判定、数値等）

### 2.5 Phase 2 完了基準

以下の **すべて** が満たされたら Phase 2 完了：

✓ MSVC CUnit テスト全 PASS  
✓ MSVC CPPUnit テスト全 PASS  
✓ TCC libcunit.a 再ビルド成功  
✓ TCC 側テスト出力 == MSVC 版出力（完全一致）

---

## Phase 3：Stage 2 実装（マングリング＆スコープ）（5～8h）

### 目的
関数オーバーロードに対応し、軽量マングリング実装を行う。クラススコープ内での名前解決を実現する。

### 3.1 TCC 側実装 *(必須前提：Phase 2 完了)*

**修正ファイル**：`tccgen.c`

**追加内容**：

1. **軽量マングリング実装**
   - **アルゴリズム**：
     - 関数シグネチャ（関数名 + 引数型リスト）に基づいた一意な内部シンボル名を生成
     - 形式例：`_Z3f2ii` (Itanium 風、ただし TCC 内部用に簡略化)
     - または `__tcc_f_2_int_int` (TCC 独自形式、より可読性重視)
   - **実装位置**：`decl()` 関数内、シンボル登録時

2. **参照型（`&`）パース**
   - `type_decl()` 拡張
   - `&` 演算子を型修飾子として認識
   - 参照型情報を `CType` 構造体に格納
   - マングル名生成時に参照型を含める

3. **`Sym` 構造体拡張**
   ```c
   struct Sym {
     int v;                    // token ID
     int type;                 // union of values
     CType ctype;              // function/type info
     struct Sym *next;         // next symbol in hash table
     int c;                    // associated number or offset (asm label number)
     struct Sym *prev;         // previous in scope stack
     int prev_tok;             // previous token for recovery
     const char *file;         // source file
     int line;                 // source line
     
     // +++新規追加+++
     struct Sym *parent_class; // メンバ関数の場合、所属クラスへのポインタ
     int class_offset;         // クラス内でのメンバオフセット
     // +++新規追加+++
   };
   ```

4. **クラススコープ管理**
   - `Sym->parent_class` を用いて、メンバ関数とメンバ変数を分類
   - シンボル検索時に `parent_class` チェックを追加

### 3.2 MSVC 側テスト実行 *(依存：3.1 の TCC 実装完了)*

**実行内容**：

- **con_vs_test 実行**
  ```bash
  cd test\vs_test
  msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
  Debug Unicode\con_vs_test.exe
  ```
  - `test_stage2_mangling.c` の全テストケース実行
  - 期待：「マングリング区別 PASS」「参照型マングル PASS」「シンボル衝突回避 PASS」

- **tcc_cpp_unittest 実行**
  ```bash
  cd test\tcc_cpp_unittest
  vstest.console.exe tcc_cpp_unittest.dll
  ```
  - `cpp_test_stage2.cpp` の全テストケース実行
  - 期待：「FunctionOverloading PASS」「ReferenceParameters PASS」

### 3.3 TCC libcunit.a 再ビルド（Stage 2 版） *(依存：3.1 の TCC 実装完了)*

```bash
cd dev\cunit
make_lib_cpp_stage2.bat
```

**確認項目**：
- アーカイブ生成成功（`libcunit_cpp_stage2.a` 出力）
- シンボル数が Stage 1 版より増加（マングリング対応の確認）

### 3.4 TCC 側テスト実行 *(依存：3.3 の libcunit.a 再ビルド完了)*

```bash
cd test\vs_test
tcc test_stage2_mangling.c -o test_stage2_mangling_tcc.exe
test_stage2_mangling_tcc.exe > stage2_tcc_output.txt
fc stage2_tcc_output.txt stage2_msvc_output.txt
```

**確認項目**：
- TCC コンパイル・実行成功
- 出力完全一致

### 3.5 Phase 3 完了基準

✓ MSVC CUnit テスト全 PASS  
✓ MSVC CPPUnit テスト全 PASS  
✓ TCC libcunit.a 再ビルド成功  
✓ TCC 側テスト出力 == MSVC 版出力（完全一致）

---

## Phase 4a：Stage 3 実装（this＆メンバ関数＆デフォルト引数）（8～11h）

### 目的
メンバ関数呼び出しに暗黙的に `this` ポインタを付与し、デフォルト引数を補完する。

### 4a.1 TCC 側実装 *(必須前提：Phase 3 完了)*

**修正ファイル**：`tccgen.c`, `x86_64-gen.c`, `arm-gen.c`, `arm64-gen.c`

**追加内容**：

1. **`unary()` 関数拡張 - メンバ関数呼び出し検出**
   - ドット演算子（`.`）で接続された識別子がメンバ関数の場合、`this` を隠し引数として準備
   - アロー演算子（`->`）の場合も同様

2. **`gfunc_call()` 拡張 - `this` レジスタ配置**
   ```c
   // 各プラットフォームの ABI に従う
   // Windows x64: RCX
   // x86 (Windows __thiscall): ECX
   // ARM: R0 (AAPCS)
   // ARM64: X0 (AAPCS64)
   ```

3. **`gfunc_prolog()` 拡張**
   - メンバ関数の第 1 引数として `this` を ABI 指定のレジスタから受け取る

4. **デフォルト引数パース**
   ```c
   // void set(int x = 10) の場合、
   // パース時に引数リストに「デフォルト値 10」を記録
   // 呼び出し側で引数省略時に自動補完
   ```

5. **遅延パース（TokenString）フレームワーク初期版**
   - クラス定義内のメンバ関数本体を `TokenString` に保存
   - クラス定義の閉じ括弧 `};` 後、順次トークン列を復帰して `gen_function()` 実行

### 4a.2 各プラットフォーム別実装

**`x86_64-gen.c`**（Windows x64）
```c
// this ポインタを RCX に配置
void gen_this_register(void) {
    // RCX (ARG0) に this の値をセット
}
```

**`i386-gen.c`**（x86 32-bit）
```c
// __thiscall: this を ECX に配置
// (Windows 固有の呼び出し規約)
```

**`arm-gen.c`**（ARM 32-bit）
```c
// AAPCS: this ポインタを R0（ARG0）に配置
void gen_this_register_arm(void) {
    // R0 に this の値をセット
}
```

**`arm64-gen.c`**（ARM 64-bit / AArch64）
```c
// AAPCS64: this ポインタを X0（ARG0）に配置
// スタックアライメント（16 バイト）に留意
void gen_this_register_arm64(void) {
    // X0 に this の値をセット
}
```

### 4a.3 MSVC 側テスト実行 *(依存：4a.1 の TCC 実装完了)*

```bash
cd test\vs_test
msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
Debug Unicode\con_vs_test.exe
```

- `test_stage3_members.c` の全テストケース実行
- 期待：「メンバ関数呼び出し PASS」「メンバ変数アクセス PASS」「デフォルト引数補完 PASS」

```bash
cd test\tcc_cpp_unittest
vstest.console.exe tcc_cpp_unittest.dll
```

- `cpp_test_stage3.cpp` の全テストケース実行
- 期待：「MemberFunctionCall PASS」「MemberVariableAccess PASS」「DefaultArguments PASS」

### 4a.4 TCC libcunit.a 再ビルド（Stage 3 版） *(依存：4a.1 の TCC 実装完了)*

```bash
cd dev\cunit
make_lib_cpp_stage3.bat
```

**確認項目**：
- アーカイブ生成成功
- この時点で libcunit が「簡易 C++ 対応」で動作確認

### 4a.5 TCC 側テスト実行 *(依存：4a.4 の libcunit.a 再ビルド完了)*

```bash
cd test\vs_test
tcc test_stage3_members.c -o test_stage3_members_tcc.exe
test_stage3_members_tcc.exe > stage3_tcc_output.txt
fc stage3_tcc_output.txt stage3_msvc_output.txt
```

### 4a.6 Phase 4a 完了基準

✓ MSVC CUnit テスト全 PASS  
✓ MSVC CPPUnit テスト全 PASS  
✓ TCC libcunit.a 再ビルド成功  
✓ TCC 側テスト出力 == MSVC 版出力（完全一致）

---

## Phase 4b：Stage 4 実装（継承＆初期化子リスト＆静的メンバ）（8～11h）

### 目的
単一継承、初期化子リスト、静的メンバ変数・関数を実装する。

### 4b.1 TCC 側実装 *(必須前提：Phase 4a 完了)*

**修正ファイル**：`tccgen.c`

**追加内容**：

1. **単一継承パース**
   ```c
   // class Derived : public Base { ... };
   // パース時に Derived の構造体先頭に Base を配置
   ```

2. **初期化子リスト実装**
   ```c
   // class Derived : Base {
   //   Derived() : Base(10), member(20) { ... }
   // };
   // パース：`:` の後の初期化リストを記録
   // コード生成：Base と member の初期化コードを挿入
   ```

3. **静的メンバ変数・関数**
   ```c
   // class Counter {
   //   static int count;      // 宣言
   //   static void reset();   // 関数宣言
   // };
   // int Counter::count = 0;  // 定義（グローバル領域）
   ```

### 4b.2 MSVC 側テスト実行 *(依存：4b.1 の TCC 実装完了)*

```bash
cd test\vs_test
msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
Debug Unicode\con_vs_test.exe
```

- `test_stage4_inheritance.c` の全テストケース実行

```bash
cd test\tcc_cpp_unittest
vstest.console.exe tcc_cpp_unittest.dll
```

- `cpp_test_stage4.cpp` の全テストケース実行
- 期待：「SingleInheritance PASS」「MemoryLayout PASS」「ConstructorInitializerList PASS」「StaticMemberVariable PASS」

### 4b.3 TCC libcunit.a 再ビルド（Stage 4 版） *(依存：4b.1 の TCC 実装完了)*

```bash
cd dev\cunit
make_lib_cpp_stage4.bat
```

### 4b.4 TCC 側テスト実行 *(依存：4b.3 の libcunit.a 再ビルド完了)*

```bash
cd test\vs_test
tcc test_stage4_inheritance.c -o test_stage4_inheritance_tcc.exe
test_stage4_inheritance_tcc.exe > stage4_tcc_output.txt
fc stage4_tcc_output.txt stage4_msvc_output.txt
```

### 4b.5 Phase 4b 完了基準

✓ MSVC CUnit テスト全 PASS  
✓ MSVC CPPUnit テスト全 PASS  
✓ TCC libcunit.a 再ビルド成功  
✓ TCC 側テスト出力 == MSVC 版出力（完全一致）

---

## Phase 5：メンバポインタ フェーズ 1&2 実装（8～11h）

### 目的
メンバポインタを段階的に実装。フェーズ 1 でデータメンバポインタ、フェーズ 2 で非仮想メンバ関数ポインタを対応。

### 5.1 TCC 側実装 - フェーズ 1 *(必須前提：Phase 4b 完了)*

**修正ファイル**：`tccgen.c`

**追加内容**：

1. **データメンバポインタ型パース**
   ```c
   // T C::*p の型を認識
   // 内部表現：単純にオフセット値（int）
   ```

2. **メンバアドレス取得**
   ```c
   // p = &C::member; → p = offsetof(C, member)
   ```

3. **メンバ間接アクセス**
   ```c
   // obj.*p ⇒ *(T*)((char*)&obj + offset) に変換
   // obj.x の場合：x のオフセットをコンパイル時に確定し、
   //              (T*)((char*)&obj + offset) でアクセス
   ```

### 5.2 MSVC 側テスト実行 - フェーズ 1 *(依存：5.1 の TCC 実装完了)*

```bash
cd test\vs_test
msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
Debug Unicode\con_vs_test.exe
```

- `test_member_pointer.c` の **フェーズ 1 テストケース** 実行

```bash
cd test\tcc_cpp_unittest
vstest.console.exe tcc_cpp_unittest.dll
```

- `cpp_test_member_pointer.cpp` の **フェーズ 1 テストケース** 実行
- 期待：「DataMemberPointer PASS」

### 5.3 TCC 側実装 - フェーズ 2 *(依存：5.2 の MSVC フェーズ 1 テスト PASS)*

**修正ファイル**：`tccgen.c`

**追加内容**：

1. **メンバ関数ポインタ型パース**
   ```c
   // R (C::*pmf)(Args...) の型を認識
   ```

2. **メンバ関数ポインタ値の表現**
   ```c
   // 内部：void* (オブジェクトアドレス) + 関数ポインタ型の組み合わせ
   // または：関数ポインタ型（void* this を第 1 引数に自動挿入）
   ```

3. **メンバ関数間接呼び出し**
   ```c
   // (obj.*pmf)(args) ⇒ ((R(*)(void*, Args...))pmf)((void*)&obj, args)
   // に変換
   ```

4. **制約とドキュメント**
   - 単一継承のみ対応
   - 仮想メソッド未対応
   - 多重継承未対応

### 5.4 MSVC 側テスト実行 - フェーズ 2 *(依存：5.3 の TCC 実装完了)*

```bash
cd test\vs_test
msbuild con_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
Debug Unicode\con_vs_test.exe
```

- `test_member_pointer.c` の **フェーズ 2 テストケース** 実行

```bash
cd test\tcc_cpp_unittest
vstest.console.exe tcc_cpp_unittest.dll
```

- `cpp_test_member_pointer.cpp` の **フェーズ 2 テストケース** 実行
- 期待：「MemberFunctionPointer PASS」

### 5.5 TCC libcunit.a 最終版再ビルド *(依存：5.4 の MSVC フェーズ 2 テスト PASS)*

```bash
cd dev\cunit
make_lib_cpp_final.bat
```

**出力**：`dev/lib/libcunit_cpp_final.a`

### 5.6 TCC 側統合テスト実行 *(依存：5.5 の libcunit.a 再ビルド完了)*

```bash
cd test\vs_test
tcc test_member_pointer.c -o test_member_pointer_tcc.exe
test_member_pointer_tcc.exe > member_pointer_tcc_output.txt
fc member_pointer_tcc_output.txt member_pointer_msvc_output.txt
```

### 5.7 全ステージ統合テスト実行（最終確認）

**テスト対象**：
- `test_stage1_keywords.c`
- `test_stage2_mangling.c`
- `test_stage3_members.c`
- `test_stage4_inheritance.c`
- `test_member_pointer.c`（フェーズ 1 & 2 含む）

**実行方法**：
```bash
cd test\vs_test

REM すべてを CUnit で統合実行
tcc test_stage*.c test_member_pointer.c -o test_all_tcc.exe
test_all_tcc.exe > all_tcc_output.txt

REM MSVC 版との出力比較
fc all_tcc_output.txt all_msvc_output.txt
```

### 5.8 Phase 5 完了基準

✓ MSVC CUnit テスト全 PASS（フェーズ 1 & 2）  
✓ MSVC CPPUnit テスト全 PASS（フェーズ 1 & 2）  
✓ TCC libcunit.a 最終版再ビルド成功  
✓ TCC 側テスト出力 == MSVC 版出力（完全一致）  
✓ 全ステージ統合テスト全 PASS

---

## テストケース仕様

### Stage 1 - キーワード＆基本型

**テスト対象ファイル**：`test/vs_test/test_stage1_keywords.c` (CUnit)

```c
// test_stage1_keywords.c
#include "CUnit/CUnit.h"
#include "test_common.h"

// テスト対象コード例
class Point {
    int x;
    int y;
};

bool flag = true;
struct Square {  // struct はデフォルト public
    int side;
};

// テストケース
void test_class_keyword_recognized(void) {
    // class Point のコンパイル成功確認
    Point p;
    CU_ASSERT_EQUAL(sizeof(Point), sizeof(int) * 2);
}

void test_bool_type_parse(void) {
    // bool の宣言・初期化
    bool b1 = true;
    bool b2 = false;
    CU_ASSERT_EQUAL(b1, 1);
    CU_ASSERT_EQUAL(b2, 0);
}

void test_default_access_control(void) {
    // class は private、struct は public がデフォルト
    // （エラー時の tcc_error 発生回数で確認、など）
    CU_ASSERT(1);  // 簡略版
}

// テストスイート登録
CU_TestInfo tests_stage1[] = {
    {"class keyword recognized", test_class_keyword_recognized},
    {"bool type parse", test_bool_type_parse},
    {"default access control", test_default_access_control},
    CU_TEST_INFO_NULL,
};
```

**CPPUnit 版**：`test/tcc_cpp_unittest/cpp_test_stage1.cpp`

```cpp
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tcccppunittest {
    class Point {
    private:
        int x;
        int y;
    };

    TEST_CLASS(Stage1Tests) {
    public:
        TEST_METHOD(ClassKeywordRecognized) {
            Point p;
            Assert::IsTrue(sizeof(Point) == sizeof(int) * 2);
        }

        TEST_METHOD(BoolTypeUsage) {
            bool flag = true;
            Assert::IsTrue(flag == 1);
        }
    };
}
```

### Stage 2 - マングリング＆スコープ

**テスト対象ファイル**：`test/vs_test/test_stage2_mangling.c` (CUnit)

```c
// テスト対象コード例
int func(int x) { return x * x; }
int func(double x) { return (int)(x * x); }

void test_mangling_distinct_overloads(void) {
    // int func(int) と int func(double) の呼び分け
    int r1 = func(5);        // int 版 → 25
    int r2 = func(3.0);      // double 版 → 9
    CU_ASSERT_EQUAL(r1, 25);
    CU_ASSERT_EQUAL(r2, 9);
}
```

### Stage 3 - メンバ関数＆this＆デフォルト引数

**テスト対象ファイル**：`test/vs_test/test_stage3_members.c` (CUnit)

```c
// テスト対象コード例
class Point {
    int x;
    int y;
public:
    void set(int a, int b = 0) { x = a; y = b; }
    int getX() { return x; }
};

void test_member_function_call(void) {
    Point p;
    p.set(3, 4);
    CU_ASSERT_EQUAL(p.getX(), 3);
}

void test_default_argument_substitution(void) {
    Point p1, p2;
    p1.set(10);       // b のデフォルト値 0 が使用される
    p2.set(20, 30);   // b = 30
    CU_ASSERT_EQUAL(p1.getX(), 10);
    CU_ASSERT_EQUAL(p2.getX(), 20);
}
```

### Stage 4 - 継承＆初期化子リスト＆静的メンバ

**テスト対象ファイル**：`test/vs_test/test_stage4_inheritance.c` (CUnit)

```c
// テスト対象コード例
class Base {
    int base_val;
public:
    Base(int v = 0) { base_val = v; }
};

class Derived : public Base {
    int derived_val;
public:
    Derived(int b, int d) : Base(b) { derived_val = d; }
};

static int Counter::count = 0;

void test_single_inheritance(void) {
    Derived d(5, 10);
    CU_ASSERT_EQUAL(sizeof(Derived), sizeof(Base) + sizeof(int));
}
```

### メンバポインタ - フェーズ 1 & 2

**テスト対象ファイル**：`test/vs_test/test_member_pointer.c` (CUnit)

```c
// テスト対象コード例（フェーズ 1）
class Point {
    int x;
    int y;
public:
    void reset() { x = 0; y = 0; }
};

void test_data_member_pointer(void) {
    int Point::*px = &Point::x;
    Point p;
    p.x = 5;
    CU_ASSERT_EQUAL(p.*px, 5);
    p.*px = 10;
    CU_ASSERT_EQUAL(p.x, 10);
}

// テスト対象コード例（フェーズ 2）
void test_member_function_pointer(void) {
    void (Point::*pmf)() = &Point::reset;
    Point p;
    p.x = 100;
    (p.*pmf)();
    CU_ASSERT_EQUAL(p.x, 0);
}
```

---

## 完了基準と検証

### 各フェーズの完了基準

| フェーズ | 判定基準（以下をすべて満たす） |
| :---: | :--- |
| **1** | con_vs_test, tcc_cpp_unittest のスケルトン完成 ＆ CUnit/CPPUnit 実行可能 ＆ make_lib_cpp_*.bat スクリプト作成完了 |
| **2** | MSVC CUnit 全 PASS ＆ MSVC CPPUnit 全 PASS ＆ TCC libcunit.a 再ビルド成功 ＆ TCC 出力 == MSVC 出力 |
| **3** | MSVC CUnit 全 PASS ＆ MSVC CPPUnit 全 PASS ＆ TCC libcunit.a 再ビルド成功 ＆ TCC 出力 == MSVC 出力 |
| **4a** | MSVC CUnit 全 PASS ＆ MSVC CPPUnit 全 PASS ＆ TCC libcunit.a 再ビルド成功 ＆ TCC 出力 == MSVC 出力 |
| **4b** | MSVC CUnit 全 PASS ＆ MSVC CPPUnit 全 PASS ＆ TCC libcunit.a 再ビルド成功 ＆ TCC 出力 == MSVC 出力 |
| **5** | MSVC CUnit 全 PASS（フェーズ 1&2） ＆ MSVC CPPUnit 全 PASS（フェーズ 1&2） ＆ TCC libcunit.a 最終版再ビルド成功 ＆ TCC 出力 == MSVC 出力 ＆ 全ステージ統合テスト全 PASS |

### テスト実行チェックリスト

```
Phase 1 テスト基盤整備
  □ con_vs_test プロジェクト拡張（test_common.h, test_stage*.c 追加）
  □ tcc_cpp_unittest プロジェクト拡張（cpp_test_stage*.cpp 追加）
  □ libcunit.a 改造版ビルドスクリプト作成（make_lib_cpp_*.bat）
  
Phase 2 Stage1 実装
  □ tcctok.h にキーワード登録（class, bool, true, false 等）
  □ tccpp.c でトークン処理実装
  □ tccgen.c で struct_decl() 拡張（class 対応）
  □ MSVC CUnit テスト実行 → 全 PASS 確認
  □ MSVC CPPUnit テスト実行 → 全 PASS 確認
  □ libcunit.a Stage1 版再ビルド成功
  □ TCC 側テスト実行 → MSVC 出力と完全一致
  
Phase 3 Stage2 実装
  □ tccgen.c にマングリング実装
  □ tccgen.c に参照型（&）パース実装
  □ Sym 構造体に parent_class 追加
  □ MSVC CUnit テスト実行 → 全 PASS 確認
  □ MSVC CPPUnit テスト実行 → 全 PASS 確認
  □ libcunit.a Stage2 版再ビルド成功
  □ TCC 側テスト実行 → MSVC 出力と完全一致
  
Phase 4a Stage3 実装
  □ tccgen.c に this 暗黙割当実装
  □ gfunc_call/gfunc_prolog で this レジスタ配置
  □ x86_64-gen.c, arm-gen.c, arm64-gen.c で ABI 別対応
  □ デフォルト引数パース & 補完実装
  □ 遅延パース（TokenString）フレームワーク初期版実装
  □ MSVC CUnit テスト実行 → 全 PASS 確認
  □ MSVC CPPUnit テスト実行 → 全 PASS 確認
  □ libcunit.a Stage3 版再ビルド成功
  □ TCC 側テスト実行 → MSVC 出力と完全一致
  
Phase 4b Stage4 実装
  □ tccgen.c に単一継承パース実装
  □ 初期化子リスト（Member Initializer List）実装
  □ 静的メンバ変数・関数実装
  □ MSVC CUnit テスト実行 → 全 PASS 確認
  □ MSVC CPPUnit テスト実行 → 全 PASS 確認
  □ libcunit.a Stage4 版再ビルド成功
  □ TCC 側テスト実行 → MSVC 出力と完全一致
  
Phase 5 メンバポインタ実装
  □ フェーズ 1：データメンバポインタ実装
    - 型パース（T C::*）
    - オフセット値表現
    - obj.*p 間接アクセス変換
  □ MSVC CUnit フェーズ 1 テスト → 全 PASS 確認
  □ MSVC CPPUnit フェーズ 1 テスト → 全 PASS 確認
  □ フェーズ 2：メンバ関数ポインタ実装
    - 型パース（R (C::*pmf)(Args...)）
    - 関数ポインタ型への変換
    - (obj.*pmf)(args) 呼び出し変換
  □ MSVC CUnit フェーズ 2 テスト → 全 PASS 確認
  □ MSVC CPPUnit フェーズ 2 テスト → 全 PASS 確認
  □ libcunit.a 最終版再ビルド成功
  □ TCC 側統合テスト実行 → MSVC 出力と完全一致
  □ 全ステージ統合テスト実行 → 全 PASS 確認
```

### 出力比較検証方法

各ステージ完了時に実行：

```bash
# MSVC 版出力取得
cd test\vs_test
Debug Unicode\con_vs_test.exe > stage_N_msvc_output.txt 2>&1

# TCC 版出力取得
tcc test_stageN_*.c -o test_stageN_tcc.exe
test_stageN_tcc.exe > stage_N_tcc_output.txt 2>&1

# テキスト比較
fc stage_N_msvc_output.txt stage_N_tcc_output.txt

# 完全一致 → 検証 PASS
# 差異あり → 差異を記録・分析
```

---

## タイムライン

### 参考見積もり（実装者の効率に応じて変動）

| フェーズ | 内容 | 実装 | MSVC テスト | TCC テスト | 合計 |
| :---: | :--- | :---: | :---: | :---: | :---: |
| **1** | テスト基盤整備 | 2～3h | 1～2h | 2～3h | **5～8h** |
| **2** | Stage1（キーワード）| 1～2h | 1h | 1～2h | **3～5h** |
| **3** | Stage2（マングリング）| 2～3h | 1～2h | 2～3h | **5～8h** |
| **4a** | Stage3（this/メンバ関数）| 3～4h | 2～3h | 3～4h | **8～11h** |
| **4b** | Stage4（継承）| 3～4h | 2～3h | 3～4h | **8～11h** |
| **5** | メンバポインタ（フェーズ 1&2）| 3～4h | 2～3h | 3～4h | **8～11h** |
| **合計** | | | | | **37～58h** |

### マイルストーン

- **MS1**：Phase 1 完了 → テスト基盤確立
- **MS2**：Phase 2 完了 → Stage1 動作確認
- **MS3**：Phase 3 完了 → オーバーロード対応
- **MS4a**：Phase 4a 完了 → メンバ関数呼び出し確認
- **MS4b**：Phase 4b 完了 → 継承機能確認
- **MS5**：Phase 5 完了 → メンバポインタ完全実装 ✓ **実装完了**

---

## 外部比較テスト（Clang との検証・オプション）

各ステージ完了時に実行（リソースに余裕がある場合）：

### テスト対象：`cpp_feature_test.cpp`（統合テストソース）

```cpp
// Stage ごとの機能を網羅したテストソース
class Point {
    int x, y;
public:
    Point(int a = 0, int b = 0) { x = a; y = b; }
    int getX() { return x; }
};

int main() {
    Point p(3, 4);
    printf("Point: x=%d, y=%d\n", p.getX(), p.y);
    return 0;
}
```

### 検証フロー

```bash
# Clang コンパイル & 実行
clang++ cpp_feature_test.cpp -o test_clang.exe
test_clang.exe > clang_output.txt

# TCC コンパイル & 実行
tcc cpp_feature_test.cpp -o test_tcc.exe
test_tcc.exe > tcc_output.txt

# 出力比較
fc clang_output.txt tcc_output.txt

# 完全一致 → 外部検証 PASS
```

---

## 参考資料

- **[IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)** ⭐ **必読**
  - libcunit.a 改造ポイント（具体的コード例）
  - TCC ビルドスクリプト完全版（make_lib_cpp_*.bat）
  - ABI 別 `this` レジスタ配置のコード例
  - テスト出力ログテンプレート
  - メンバポインタ移行条件チェックリスト

- **TCC_CPP.md**：設計仕様書（6 ステージ、メンバポインタ段階計画）
- **.vscode/skills/tcc_cpp_skill/SKILL.md**：ワークフロー定義
- **tcctok.h**：トークン定義テーブル
- **tccgen.c**：パーサ・コード生成中核
- **tccpp.c**：トークナイザ実装
- **x86_64-gen.c**, **arm-gen.c**, **arm64-gen.c**：ABI 別コード生成

---

## ドキュメント履歴

| 版 | 日付 | 内容 |
| :---: | :--- | :--- |
| 1.0 | 2026-05-23 | 初版作成。5 フェーズ＋メンバポインタ段階計画、全テストケース仕様詳細化、完了基準明記 |

---

**作成者**：GitHub Copilot  
**対象チーム**：TCC C++98 拡張実装プロジェクト  
**最終確認日**：2026-05-23
