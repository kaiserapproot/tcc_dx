# tpp (TCC 拡張版) C++ 機能説明書

**対象**: `dev\tcc.exe`（tcc version 0.9.28rc、x86_64 Windows）
**基準コミット**: `f593cb2`（2026-09-24。BUG-52/53 修正を含む。行番号はこのコミット時点の値）
**位置づけ**: TCC (Tiny C Compiler) に C++98 の一部機能を追加したもの。C++ コンパイラの置き換えではない。

<!-- doc-sample-qualification: この行はサンプル検証スクリプトがこのファイルを見つけるための ASCII マーカー。消さないこと。 -->

---

## 0. この文書の読み方

### 0.1 目的

- 本プロジェクトが実装した C++ 機能を、機能ごとに「できること」「実装箇所」「amateras での用途」「サンプル」「制限」の順で説明する。
- 実装箇所は、すべてソースを直接確認した上で記載している。行番号は基準コミット時点の値である。
- **行番号の参照は機械的に監査している**（`DOC_SOURCE_REFS_TOTAL=235` / `INVALID_RANGE=0` / `OUT_OF_FILE=0` / `NAME_CHECKED=153` / `NAME_MISMATCH=0` / `EXTERNAL=41`）。範囲の始点が終点を超えていないか、ファイルの行数を超えていないか、関数名を併記した参照がその関数の中を指しているかを `dev\test\a9\manual\doc_source_refs_check.c` が確認する。実行は `dev\test\a9\manual\doc_cpp_samples_measure.bat`。`tccgen.c` を触ったら必ず回すこと。
- amateras での用途は、`E:\work\work_cross_platform\kaiser_system\amateras` のソース（`base_inc/**`、`test/tcc/**`）を直接確認した上で記載している。

### 0.2 関連文書

| 文書 | 内容 |
|---|---|
| [実装済み.md](実装済み.md) | 機能一覧の正本（実装日・テスト名つき） |
| [tpp仕様.md](tpp仕様.md) | 動作しない機能の実測結果 |
| [CPP_CAPABILITY_MATRIX.md](CPP_CAPABILITY_MATRIX.md) | 機能ごとの対応状況の一覧表 |
| [amateras対応作業履歴.md](amateras対応作業履歴.md) | amateras を C++ としてビルドした経緯 |
| [問題と原因.md](問題と原因.md) | 過去のバグと原因 |

### 0.3 主な実装ファイル

| ファイル | 役割 |
|---|---|
| `tccgen.c` | 構文解析とコード生成。C++ 機能の大部分はここにある（`cpp_` で始まる関数が C++ 用） |
| `tccpp.c` | 字句解析とマクロ処理。C++ キーワードの扱いと `__cplusplus` の定義 |
| `tcc.h` | 共通の型定義。C++ 用の型フラグ（参照、メンバポインタ、`thread_local`）とシンボルの追加フィールド |
| `tcctok.h` | キーワード一覧。`class`、`virtual`、`this`、`operator` などの C++ キーワード |
| `libtcc.c` | 拡張子による C / C++ の判定、`-x c++` オプション |
| `tccelf.c` | グローバルオブジェクトの起動コードと `thread_local` の実行時コードの組み込み |
| `tccpe.c` | Windows 実行ファイル生成時のエントリ差し替えと TLS ディレクトリの出力 |
| `tccrun.c` | `-run`（メモリ上で即実行）時の `thread_local` 対応 |

### 0.4 サンプルの前提

- サンプルはすべて `.cpp` として `dev\tcc.exe sample.cpp -o sample.exe` でビルドできる形で書いている。
- 実行結果は終了コードで確認できるようにしている（`0` が成功）。
- 各サンプルは `dev/test/a7/`、`dev/test/a8/`、`dev/test/a9/` の回帰テスト（`run_all.bat` でビルドと実行を確認しているもの）の形を元に作成した。
- **本文書のサンプルは全 26 本を `dev\tcc.exe` でビルドして実行し、すべて終了コード 0 になることを確認している**（`DOC_CPP_SAMPLES_TOTAL=26` / `COMPILE_PASS=26` / `RUNTIME_PASS=26` / `FAIL=0`）。確認は `dev\test\a9\manual\doc_cpp_samples_measure.bat` が行う。Markdown の ```cpp / ```c ブロックのうち `int main` を含むものを `dev\test\a9\manual\doc_sample_extract.c` が抜き出し、`.cpp`（C++ ブロック）と `.c`（C ブロック）に振り分けてビルド・実行する。サンプルを増やしたら同じスクリプトを回すこと。

---

## 1. 全体像

### 1.1 C++ として処理される条件

| 条件 | 動作 |
|---|---|
| 拡張子が `.cpp` / `.cxx` / `.cc` / `.hpp` | C++ として処理する |
| `-x c++` または `-x c++-header` を指定 | 拡張子に関係なく C++ として処理する |
| 拡張子が `.h` | それを取り込んだ元のソースファイルのモードに従う |
| 拡張子が `.c` | 常に C として処理する。`class` などの C++ キーワードは普通の識別子として扱う |

`tcc foo.cpp bar.c` のように C と C++ を混ぜてビルドできる。

**実装箇所**

- `libtcc.c:1218` `is_cpp_source()` — 拡張子の判定
- `libtcc.c:1228` `guess_filetype()` — ファイル種別の決定（`.cpp` などを C 系ソースとして扱う）
- `libtcc.c:825-827` — ファイルごとに `s1->cpp` を設定
- `libtcc.c:2157-2158` — `-x c++` の処理（`cpp_forced`）
- `tccpp.c:3710` — C++ のときだけ `#define __cplusplus 199711L` を定義

### 1.2 名前修飾（リンク名の付け方）

C++ では同じ名前の関数を複数定義できるため、リンク名に引数の型を付けて区別する。本プロジェクトは独自の形式を使う。

| 対象 | リンク名 |
|---|---|
| 自由関数 | `__tcc_<関数名>_<引数型の並び>` |
| メンバ関数 | `__tcc_<クラス名>__<関数名>_<引数型の並び>`（const メンバ関数は末尾に `_C`） |
| コンストラクタ | `__cpp_ctor_<クラス名>` |
| デストラクタ | `__cpp_dtor_<クラス名>` |
| 仮想関数表 | `__cpp_vtbl_<クラス名>`、多重継承の 2 番目以降は `__cpp_vtbl2_<派生>_<基底>` |

`main` / `wmain` と `extern "C"` を付けた宣言は修飾しない。

**実装箇所**

- `tccgen.c:361` `cpp_build_func_mangle()` — 宣言側のリンク名生成
- `tccgen.c:398` `cpp_build_call_mangle()` — 呼び出し側のリンク名生成
- `tccgen.c:1095` `cpp_ctor_name_tok()`、`tccgen.c:1123` `cpp_dtor_name_tok()` — コンストラクタ / デストラクタの名前

**制限**

- 他の C++ コンパイラ（MSVC、GCC）が生成したオブジェクトファイルとはリンクできない。外部ライブラリと繋ぐときは `extern "C"` の関数を経由する。

### 1.3 実行形態ごとの対応

| 実行形態 | 状況 |
|---|---|
| 通常の実行ファイル（`-o foo.exe`） | 全機能が対象 |
| `-run`（メモリ上で即実行） | グローバルオブジェクトの構築、`thread_local` を含めて対象 |
| ライブラリとして組み込み（libtcc） | `tcc_run()` 経由は対象。再配置だけして手動で `main` を呼ぶ形は後始末が呼び出し側の責任 |
| DLL（`-shared`） | グローバルオブジェクトの構築、`thread_local` は対象外（`thread_local` はコンパイルエラーになる） |

---

## 2. 字句とキーワード

### 2.1 C++ キーワードと C ソースの共存

**できること**

- `.cpp` では `class`、`this`、`virtual`、`new`、`delete`、`operator`、`true`、`false`、`bool`、`friend`、`public` などをキーワードとして扱う。
- `.c` では同じ単語を普通の識別子として扱う。たとえば C ソースの `int class;` や `int new;` はそのまま通る。
- 1 回の実行で `.cpp` と `.c` を混ぜても、それぞれのモードで正しく処理する。

**実装箇所**

- `tcctok.h:56-61`、`163-176` — C++ キーワードの登録（`class`、`friend`、`new`、`delete`、`public`、`virtual`、`this`、`operator`、`true`、`false`、`bool`）
- `tccpp.c:462` `effective_cpp_lex()` — 現在 C++ の字句解析をしてよいかの判定
- `tccpp.c:471` `is_cpp_only_keyword()` — C では識別子に落とすキーワードの一覧
- `tccpp.c:497` `tok_alloc_demote()`、`tccpp.c:526` `demote_cpp_keyword_to_ident()` — キーワードを識別子に落とす処理
- `tccpp.c:2801` — 識別子を読んだ直後にキーワードか識別子かを決める箇所
- `tcc.h:452` `TokenSym.alt_ident_tok` — 識別子に落としたときの代替トークンを記憶する

**amateras での用途**

- amateras は C と C++ の両方でビルドできるヘッダ（`inc/UTF_8/cross.h`）を持つ。C 側のソース（`test/tcc/viewer/model_view.c` など）と C++ 側のソース（`test/tcc/n7_06/test_mmd_cross_cpp_compile.cpp` など）が同じヘッダを使うため、この共存が前提になっている。

**サンプル**（`.c` 側で C++ キーワードを識別子に使う）

```c
/* keyword_in_c.c : C として処理されるので class は識別子 */
int main(void)
{
    int class = 1;
    int new = 2;
    class++;
    return class + new - 4;   /* 0 */
}
```

### 2.2 `extern "C"`

**できること**

- `extern "C" { ... }` のブロック
- `extern "C" void f();` の単一宣言
- どちらも内側の宣言はリンク名を修飾しない。

**実装箇所**

- `tccgen.c:19376-19423` — `decl()` 内の `extern "C"` の処理（ブロック形と単一宣言形）
- `tccgen.c:344` `cpp_repromote_stale_lookahead()` — `extern "C"` を抜けた直後のトークンを C++ キーワードに戻す
- `tcc.h:817` `lex_c` — `extern "C"` の内側で C の字句解析に切り替えるカウンタ
- `tccgen.c:223` `decl_once_flag` — 単一宣言形の処理

**amateras での用途**

- `base_inc/cross_define.h:225` — `extern "C" double sqrt(double x);`。これが無いと `sqrt` がリンク名修飾されて未定義になる。
- `base_inc/3d_format/fbx/fbx.h`、`base_inc/3d_format/nif/nif.h` など多数のヘッダで、C の API 宣言を `extern "C"` で囲んでいる。

**サンプル**

```cpp
// extern_c.cpp
extern "C" int c_add(int a, int b);   // 単一宣言の形
extern "C" {                          // ブロックの形
    int c_sub(int a, int b);
    int c_add(int a, int b) { return a + b; }
    int c_sub(int a, int b) { return a - b; }
}

class Point { public: int x; };       // extern "C" の直後でもクラスを書ける

int main()
{
    Point p;
    p.x = c_add(1, 2) + c_sub(5, 3);  // 3 + 2
    return p.x - 5;
}
```

**制限**

- `extern "C++" { ... }` は未対応（エラーになる）。
- `extern "C" { #include "..." }` は未対応。
- `extern "C"` の内側は C の字句解析になるため、内側に `class X;` のような前方宣言は書けない。SDK ヘッダ（`dev/include/GL/glu.h` など）はこの理由で `__TINYC__` 用の分岐を持つ。

### 2.3 `bool` / `true` / `false`

**できること**

- `bool` 型（C の `_Bool` と同じ大きさ）
- `true` / `false` は定数 `1` / `0`

**実装箇所**

- `tcctok.h:170-176` — `true`、`false`、`bool` のキーワード登録
- `tccpp.c:471` `is_cpp_only_keyword()` — C では識別子に落とす
- `tccgen.c:15217-15222` — `true` / `false` を定数として積む

**amateras での用途**

- `base_inc/vec_quat.h:505` `bool operator==(arr2& v)`、`:536` `bool operator==(arr3& v)` — 比較演算子の戻り値
- `base_inc/render/agc/agc_define.h:50` `virtual bool init()`

**サンプル**

```cpp
// bool_basic.cpp
int main()
{
    bool t = true;
    bool z = (3 < 5);
    int r = 0;
    if (!t) r += 1;
    if (!z) r += 2;
    return r + (t + z) - 2;   // 0
}
```

---

## 3. 参照

**できること**

- 関数の引数、戻り値、ローカル変数、クラスのメンバで参照 `T&` / `const T&` が使える。
- クラス型の値を `const T&` 引数に渡せる。
- 派生クラスのオブジェクトを基底クラスの参照 `B&` に束縛できる（アドレス調整を含む）。

**実装箇所**

- `tcc.h:1155` `VT_REFERENCE` — 参照型を示す型フラグ
- `tccgen.c:13915` — 宣言子で `&` を読んで `VT_REFERENCE` を付ける
- `tccgen.c:9694` `cpp_can_bind_lvalue_to_reference()` — 参照に束縛できるかの判定
- `tccgen.c:9738`、`9805` — 参照へのバインド（アドレスを格納する）
- `tccgen.c:15978-16017` — 参照変数を使うときに自動で間接参照する
- `tccgen.c:18367-18393` — ローカル参照変数の初期化（アドレス格納）

**amateras での用途**

- `base_inc/vec_quat.h:66` `vec2 operator=(const vec2& in_data)`、`:74` `vec2 operator*(vec2& in_data)` など、ベクトル演算子の引数はすべて参照で受けている。
- `base_inc/utility/txt_util.h:55` `win_txt(const win_txt &o)`、`:68` `win_txt &operator=(const win_txt &o)` — 文字列クラスのコピー処理。

**サンプル**

```cpp
// reference.cpp
class P { public: int v; };

void bump(P& r) { r.v += 1; }
int read(const P& r) { return r.v; }

int main()
{
    P a;
    a.v = 7;
    P& r = a;        // ローカル参照
    r.v += 1;        // a.v == 8
    bump(a);         // a.v == 9
    return read(a) - 9;
}
```

**制限**

- ポインタへの参照（`void*&`、`int*&`）を直接宣言する形は未対応。typedef を経由すれば書ける。

---

## 4. クラスの基本

### 4.1 クラス定義とメンバ関数

**できること**

- `class` / `struct` の定義。`struct` は既定で public、`class` は既定で private として記録する。
- アクセス指定子 `public:` / `private:` / `protected:` を受け付ける。
- メンバ関数のクラス内定義（インライン）とクラス外定義 `int Foo::bar() { ... }`。
- メンバ関数内での `this`、`this->x`、`*this`、暗黙のメンバ参照。
- クラス名をそのまま型名として使う（`Point p;`）。`typedef Point P;` も可。
- 関数の引数名を省略できる（`void f(int) {}`）。
- `friend class X;` は受け付けて読み飛ばす。
- 関数の中でのクラス定義（局所クラス）。
- 入れ子クラスのクラス外定義 `class Outer::Inner { ... };`。

**実装箇所**

- `tccgen.c:12599` `struct_decl()` — クラス本体の解析。`is_class` が 1 のとき `class`、2 のとき C++ の `struct`
- `tccgen.c:12818-12826` — アクセス指定子の記録（`cur_access`）
- `tccgen.c:12889` — `friend class X;` の読み飛ばし
- `tccgen.c:5686` `cpp_register_member_body()`、`tcc.h:607` `Sym.inline_func_str` — クラス内定義の本体をトークン列として保存
- `tccgen.c:5945` `cpp_finish_member_inlines()` — クラス定義の終了後に保存した本体をコード生成する
- `tccgen.c:893` `parse_cpp_scope_qualifier()` — クラス外定義の `Foo::bar` の解析
- `tccgen.c:19000` `gen_function()` — メンバ関数に隠し引数 `this` を付ける
- `tccgen.c:4508` `cpp_push_member_var()` — 暗黙のメンバ参照を `this->member` に置き換える
- `tccgen.c:5708` `cpp_name_unnamed_params()` — 引数名の省略
- `tccgen.c:6891` `cpp_class_sym_push()` — 局所クラスの定義シンボルをファイル全体で生かす
- `tcc.h:609` `Sym.parent_class` — メンバがどのクラスに属するか

**amateras での用途**

- `base_inc/cross_base.h` の `window_t` — C では `typedef struct`、C++ では `struct window_t` として開き、`init()`、`reg_win()`、`run()` などのウィンドウ処理を**メンバ関数**として取り込む（生成物 `inc/UTF_8/cross.h:6985` 以降。`init()` は `:7334`、`reg_win()` は `:7383`）。
- `base_inc/utility/txt_util.h` の `win_txt` — 文字列バッファをクラスとして扱う。`clear()` などのメンバ関数を持つ。
- `base_inc/vec_quat.h` の `vec2` / `vec3` / `vec4` / `matrix3` / `matrix4` — ベクトルと行列。
- `base_inc/cross_base.h:186` — `init()` 内で `new_win = this;` のように `this` を使う（C では `malloc` で確保する）。

**サンプル**

```cpp
// class_basic.cpp
class Calc {
public:
    int base;
    int add(int a, int b);            // クラス外で定義
    int twice(int a) { return a * 2; } // クラス内で定義
    int with_base(int a) { return this->base + a; }
};

int Calc::add(int a, int b) { return a + b + base; }

int main()
{
    Calc c;
    c.base = 10;
    // 12 + 4 + 15 = 31
    return c.add(1, 1) + c.twice(2) + c.with_base(5) - 31;
}
```

**制限**

- アクセス指定子は記録するだけで、`private` メンバへの外部アクセスをエラーにはしない。
- `friend` **関数**宣言はエラーになる（読み飛ばすと関数宣言そのものが消えるため）。
- 局所クラスのタグ名は関数を抜けた後もファイル全体に残るため、同じ名前の局所クラスを別の関数で定義できない。
- クラス内の `enum` 定義、`mutable`、`explicit`、C++11 のメンバ初期化子（`int v = 5;`）は未対応。

### 4.2 関数のオーバーロードとデフォルト引数

**できること**

- 同じ名前で引数の数や型が違う関数を複数定義できる（自由関数、メンバ関数、コンストラクタ）。
- デフォルト引数 `int f(int x = 10)`。静的メンバ関数の呼び出しでも適用される。
- デフォルト引数の式は、それを宣言したクラスのスコープで評価する（`= npos` のような静的メンバも解決できる）。
- `.h` で宣言して `.cpp` で定義する、あるいは別ファイルで定義するメンバ関数を前方参照で呼べる。

**実装箇所**

- `tccgen.c:512` `cpp_resolve_func_call()`、`tccgen.c:580` `cpp_resolve_free_func_call()` — 自由関数のオーバーロード解決
- `tccgen.c:412` `cpp_arg_matches_param()` — 引数と仮引数の一致判定
- `tccgen.c:10957` `cpp_score_member_overloads()`、`tccgen.c:11040` `cpp_resolve_member_func_call()` — メンバ関数のオーバーロード解決（完全一致 10 点、変換可 1 点）
- `tccgen.c:1278` `cpp_make_member_func_extern()` — 未定義のメンバ関数を外部参照として作る（前方参照の呼び出し用）
- `tccgen.c:5634` `cpp_save_default_arg()` — デフォルト引数のトークン列を保存
- `tccgen.c:5962` `cpp_apply_default_args()` — 呼び出し時に省略された引数を補う
- `tccgen.c:6046` `cpp_inherit_decl_defaults()` — 宣言側のデフォルト引数を定義側へ引き継ぐ

**amateras での用途**

- `base_inc/vec_quat.h:185` `vec3 operator*(vec3& in_data)` と `:197` `vec3 operator*(float in_data)` — 引数の型が違う同名演算子。`vec3 b = a * 2.0f;` で `float` 版が選ばれる（`test/tcc/vec_quat/vec_quat_matrix_cpp.cpp:48`）。
- `inc/UTF_8/cross.h:3113-3129` `win_txt` — 引数なし、`const AP_STR*`、`int` の 3 種類のコンストラクタ。

**サンプル**

```cpp
// overload_default.cpp
int f(int x)          { return 1; }
int f(double x)       { return 2; }
int f(const char* s)  { return 3; }

class Foo {
public:
    static int bar(int x = 10);
};
int Foo::bar(int x) { return x; }

int main()
{
    int r = f(1) + f(2.0) + f("s");   // 1 + 2 + 3 = 6
    return r + Foo::bar() - 16;       // 6 + 10 = 16
}
```

**制限**

- 候補の順位付けは「完全一致」と「変換可」の 2 段階だけ。同点のときは先に宣言した方を選び、曖昧エラーは出さない。
- 引数名を省略したメンバ関数のクラス外定義はエラーになる。

### 4.3 `const` メンバ関数

**できること**

- `int get() const;` の宣言と定義。
- `const` オブジェクトから `const` メンバ関数を呼べる。
- `get()` と `get() const` を両方定義してオーバーロードできる。
- `const` メンバ関数の中でメンバに代入するとエラーになる。

**実装箇所**

- `tcc.h:534` `FuncAttr.func_const` — const メンバ関数を示すフラグ
- `tccgen.c:10869` `cpp_find_field_for_call()` — オブジェクトの const 性に合うメンバ関数を選ぶ
- `tccgen.c:361` `cpp_build_func_mangle()` — リンク名末尾の `_C`
- `tccgen.c:19000` `gen_function()` — `this` の型を `const T*` にする

**amateras での用途**

- `base_inc/vec_quat.h` の演算子は const を付けていない。amateras のソースでは `const` メンバ関数の使用は確認できなかった（CPPUnit の移植 `sample/cppunit` で使用）。

**サンプル**

```cpp
// const_member.cpp
class C {
public:
    int x;
    int get() const;
};
int C::get() const { return x; }

int main()
{
    C c;
    c.x = 2;
    const C& rc = c;
    return rc.get() - 2;
}
```

### 4.4 静的メンバ

**できること**

- クラス内の `static int count;` 宣言と、クラス外の `int Foo::count = 0;` 定義。
- 静的メンバ関数（`this` を持たない）。
- 式の中の `Foo::count`、`Foo::func()`、文頭の `Foo::count = 1;`。
- 同じクラスの静的メンバ関数を、定義より前の位置から修飾なしで呼べる。

**実装箇所**

- `tccgen.c:1041` `cpp_lookup_static_member()` — `Class::member` の解決
- `tccgen.c:952` `cpp_unget_scoped_expr()` — 文頭の `Class::member = ...;` を式として扱う
- `tccgen.c:19000` `gen_function()` — 静的メンバ関数には `this` を付けない
- `struct_layout()` — 静的メンバをインスタンスの大きさから除外

**amateras での用途**

- amateras のソースでは静的メンバの使用は確認できなかった（CPPUnit の移植で使用）。

**サンプル**

```cpp
// static_member.cpp
class Counter {
public:
    static int count;
    static int next();
};
int Counter::count = 0;
int Counter::next() { count += 1; return count; }

int main()
{
    Counter::next();
    Counter::next();
    Counter::count += 10;
    return Counter::count - 12;
}
```

---

## 5. コンストラクタ

### 5.1 コンストラクタの宣言・定義・呼び出し

**できること**

- クラス内定義 `Foo(int x) : a(x) {}` とクラス外定義 `Foo::Foo(int x) : a(x) {}`。
- 初期化子リスト `: a(x), b(y)`。`const` メンバの初期化も可。
- 複数のコンストラクタ（オーバーロード）。デフォルト引数付きも可。
- ローカル変数 `Foo f(5);`、複数宣言 `Foo a(1), b(2);`。
- 引数なしのコンストラクタがあるときの `Foo f;`。
- コンストラクタを持たないクラスの `Foo f;`（従来どおり領域確保のみ）。
- クラス型のメンバ変数は、初期化子リストに書かなくても自動で構築する。

**実装箇所**

- `tccgen.c:5666` `cpp_save_mem_init_list()`、`tcc.h:608` `Sym.cpp_mem_init_list` — 初期化子リストの保存
- `tccgen.c:19112-19198` — `gen_function()` 内で初期化子リストを展開
- `tccgen.c:4424` `cpp_peek_out_of_class_ctor()` — クラス外定義 `Foo::Foo(...)` の検出
- `tccgen.c:1602` `cpp_find_ctor_field()`、`tccgen.c:1647` `cpp_class_has_default_ctor()` — コンストラクタの検索と「引数なしで呼べるか」の判定
- `tccgen.c:11076` `cpp_resolve_implicit_ctor_overload()`、`tccgen.c:11106` `cpp_emit_resolved_implicit_ctor()` — 宣言 `Foo f(args);` に対するコンストラクタ選択と呼び出し
- `tccgen.c:5207` `cpp_emit_implicit_member_ctors()` — クラス型メンバの自動構築
- `tccgen.c:20077-20254` — `decl()` 内で宣言ごとに構築処理を接続する箇所

**amateras での用途**

- `base_inc/vec_quat.h:57-58` `vec2():x(0),y(0){}`、`vec2(float in_x,float in_y):x(in_x),y(in_y){}` — 引数なしと 2 引数のコンストラクタ。`vec3` / `vec4` も同様。
- `inc/UTF_8/cross.h:9607` `window_t(): handle(0), draw_context(NULL), ...` — ウィンドウ構造体の初期化。
- `base_inc/cross_base.h:29` `win_txt title;` — `window_t` のメンバ `title` は `win_txt` 型で、`window_t` の構築時に自動で構築される（`cross_base.h:186` のコメント「win_txt はコンストラクタで初期化済み」）。
- `base_inc/mmd/vmd_parse.h:44` `mmd_vmd_bone_key_t` — `vec3 pos; vec4 rot;` をメンバに持つ C 形式の `typedef struct`。C++ ではメンバのコンストラクタが自動で走る（`dev/test/a9/manual/n7_03_amateras_bone_key.cpp` が同じ形を検証）。

**サンプル**

```cpp
// ctor_basic.cpp
class Inner {
public:
    int v;
    Inner() : v(3) {}
};

class Foo {
public:
    int a;
    const int c;
    Inner in;                 // 初期化子リストに無くても自動で構築される
    Foo(int x);
    Foo(int x, int y) : a(x + y), c(9) {}
};
Foo::Foo(int x) : a(x), c(9) {}

int main()
{
    Foo f(5);
    Foo g(1, 2);
    return f.a + g.a + f.c + f.in.v - 20;   // 5 + 3 + 9 + 3 = 20
}
```

**制限**

- 引数ありのコンストラクタしか持たないクラスの `Foo f;` はコンパイルエラーになる（C++ の規則どおり）。
- 要素を明示初期化する配列 `Foo h[2] = { ... }` は未対応。
- 委譲コンストラクタ、`initializer_list` は未対応。

### 5.2 暗黙のデフォルトコンストラクタ

**できること**

- コンストラクタを宣言していないクラスでも、メンバや基底クラスにコンストラクタ付きの型があれば、それらを自動で構築する。
- メンバや基底クラスが引数なしで構築できない場合は、コンパイルエラーになる。

**実装箇所**

- `tccgen.c:5783` `cpp_synthesize_implicit_special_members()` — 暗黙のコンストラクタ / デストラクタを合成する
- `tccgen.c:5240` `cpp_can_implicit_default_ctor_exist()`、`tccgen.c:5278` `cpp_validate_implicit_default_ctor()` — 暗黙のデフォルト構築が可能かの検査
- `tccgen.c:5457` `cpp_validate_decl_default_initialization()` — 宣言時の検査
- `tccgen.c:5920` `cpp_ensure_synthetic_odr()` — 合成した関数を 1 回だけ出力する

**amateras での用途**

- `base_inc/mmd/vmd_parse.h:44` `mmd_vmd_bone_key_t` — コンストラクタを持たない `typedef struct` だが、メンバ `vec3` / `vec4` のコンストラクタが自動で呼ばれる。
- `base_inc/mmd/mmd_skinning.h` `mmd_dq_t` — `vec4 r, d;` をメンバに持つ。

**サンプル**

```cpp
// implicit_default_ctor.cpp
struct M {
    int v;
    M() : v(7) {}
};
struct H {          // コンストラクタを書いていない
    M m;
    int n;
};

int main()
{
    H h;            // M のコンストラクタが自動で呼ばれる
    return h.m.v - 7;
}
```

### 5.3 コピーコンストラクタとコピー初期化

**できること**

- ユーザー定義のコピーコンストラクタ `P(const P& o)`。
- コピー初期化 `P b = a;` と直接初期化 `P c(a);` の両方でコピーコンストラクタを呼ぶ。
- コピーコンストラクタが無いクラスは、メンバごとにコピーする（メンバにコピーコンストラクタがあればそれを呼ぶ）。
- 関数の戻り値、参照、ポインタの間接参照からの初期化も可。
- `new T(obj)` も同じ規則で構築する。

**実装箇所**

- `tccgen.c:14720` `cpp_emit_local_copy_init()` — ローカル変数のコピー初期化
- `tccgen.c:14569` `cpp_emit_copied_class_subobject()` — 使えるコピーコンストラクタがあればそれを呼ぶ
- `tccgen.c:14647` `cpp_reconstruct_copied_class_members()` — メンバごとの再構築
- `tccgen.c:19757` — `decl()` の `=` 初期化子からの接続
- `tccgen.c:14834` `cpp_emit_heap_ctor_call()` — `new T(obj)` の構築

**amateras での用途**

- `base_inc/utility/txt_util.h:55` `win_txt(const win_txt &o)` — バッファを複製するコピーコンストラクタ。
- `test/tcc/vec_quat/vec_quat_matrix_cpp.cpp:48-52` — `vec3 b = a * 2.0f;`、`vec3 e = a;` のコピー初期化。
- `base_inc/mmd/mmd_anim.h:514` `vec3 vmd_pos = VEC3_LIT(0.0f, 0.0f, 0.0f);` — C++ では `vec3(...)` の一時オブジェクトからのコピー初期化に展開される。

**サンプル**

```cpp
// copy_init.cpp
static int g_copies = 0;

struct P {
    int v;
    P() { v = 1; }
    P(const P& o) { v = o.v + 100; g_copies++; }
};

int main()
{
    P a;
    P b = a;      // コピー初期化 → b.v == 101
    P c(a);       // 直接初期化 → c.v == 101
    if (b.v != 101 || c.v != 101) return 1;
    return g_copies - 2;
}
```

**制限**

- 派生クラスのオブジェクトを基底クラスの値へコピーする形（`B b = d;`、値渡し、値返し）はエラーになる。参照 `const B&` で渡すか、直接初期化 `B b(d);` を使う。
- 関数の引数と戻り値で構造体を値渡しするときは、コピーコンストラクタを呼ばずにバイト列をコピーする。

### 5.4 変換コンストラクタの自動適用

**できること**

- 引数 1 個で呼べるコンストラクタを持つクラスへ、`return`、初期化子リスト、代入、引数渡し（値渡し・`const T&`）の場面で自動的に変換する。
- 適用は 1 段だけ。

**実装箇所**

- `tccgen.c:11154` `cpp_try_class_conversion()` — 変換コンストラクタの探索と一時オブジェクトの構築
- `tccgen.c:10290`、`10291` — 型変換の入口（`gen_assign_cast` 系）からの呼び出し

**amateras での用途**

- amateras のソースでは変換コンストラクタの自動適用は確認できなかった（CPPUnit の移植で使用）。

**サンプル**

```cpp
// convert_ctor.cpp
struct Str {
    int len;
    Str() : len(0) {}
    Str(const char* p) { len = 0; while (p[len]) len++; }
};
int take(const Str& s) { return s.len; }

int main()
{
    Str s;
    s = "abc";               // 代入で Str("abc") に変換
    return take("hello") + s.len - 8;   // 5 + 3
}
```

**制限**

- `explicit` は未対応（書けない）。

---

## 6. デストラクタと自動破棄

**できること**

- `~Foo() {}` のクラス内定義とクラス外定義 `Foo::~Foo() {}`。
- ブロックを抜けるときにローカルオブジェクトを宣言と逆順で破棄する。
- `return` で関数を抜けるときも、全スコープのローカルを破棄してから戻る（戻り値は退避する）。
- `break` / `continue` / `goto` でスコープを抜けるときも破棄する。`goto` で初期化を飛び越す形は検査してエラーにする。
- 式の中で作られた一時オブジェクトを文の終わりで破棄する。
- 派生クラスのデストラクタの後に基底クラスのデストラクタを自動で呼ぶ。クラス型メンバも逆順で自動破棄する。
- 明示呼び出し `f.~Foo()` も可。

**実装箇所**

- `tccgen.c:1789` `cpp_emit_local_dtor()` — 1 個のローカルの破棄コードを出す
- `tccgen.c:2337` `cpp_finish_scope()`、`tccgen.c:2290` `cpp_block_cleanup()` — ブロック終了時の破棄
- `tccgen.c:2350` `cpp_spill_return_value()`、`tccgen.c:2457` `cpp_restore_return_value()` — `return` 時に戻り値を退避してから破棄
- `tccgen.c:17574-17616` — `return` 文の処理からの呼び出し
- `tccgen.c:2230` `cpp_validate_goto_target()`、`tccgen.c:2248` `cpp_emit_scope_exit_dtors()` — `goto` の検査と破棄
- `tccgen.c:2123` `cpp_validate_switch_entry()` — `switch` で初期化を飛び越す形の検査
- `tccgen.c:1893` `cpp_note_class_temp()`、`tccgen.c:1975` `cpp_flush_class_temps()` — 一時オブジェクトの登録と破棄
- `tccgen.c:4824` `cpp_emit_base_dtor_calls()`、`tccgen.c:5592` `cpp_emit_member_dtor_calls()` — 基底とメンバの自動破棄
- `tccgen.c:19245-19247` — デストラクタ本体の末尾にメンバ・基底の破棄を付ける

**amateras での用途**

- `inc/UTF_8/cross.h:3129` `~win_txt() { clear(); }` — 文字列バッファの解放。
- `inc/UTF_8/cross.h:9649` `~window_t()` — スレッドハンドルの解放（`:9565` のコメント「~window_t()（C++）での二重 CloseHandle を防ぐ」）。
- `window_t` は `win_txt` 型のメンバ（`title`、`class_name`）を持つため、`~window_t()` の後にそれらのデストラクタが自動で呼ばれる。

**サンプル**

```cpp
// dtor_scope.cpp
static int log_value;

struct Guard {
    int id;
    Guard(int n) : id(n) {}
    ~Guard() { log_value = log_value * 10 + id; }
};

int work()
{
    Guard a(1);
    {
        Guard b(2);
        return 5;          // b → a の順で破棄してから 5 を返す
    }
}

int main()
{
    int r = work();        // log_value == 21
    return (r == 5 && log_value == 21) ? 0 : 1;
}
```

**制限**

- 自分でデストラクタを宣言していないが、メンバや基底のためにデストラクタ処理が要るクラス（例: デストラクタを持つクラスの配列をメンバに持つクラス）を値で返す関数はエラーになる（`return by value of a class requiring destruction is unsupported`）。**自分でデストラクタを宣言しているクラスは値で返せる**。判定は `tccgen.c:17539-17548` で、`cpp_class_requires_destruction()` が真でも `cpp_find_dtor_field()` が自クラスのデストラクタを見つければ通す。回帰テストは `dev/test/a9/negative/return_array_dtor.cpp`（エラー側）と `dev/test/a9/header_local_copy_init.cpp`（返せる側）。
- デストラクタを持つクラスの配列（ローカル配列、メンバ配列）はエラーになる。

---

## 7. グローバル・静的オブジェクト

### 7.1 グローバル変数の構築と破棄

**できること**

- `P g;`、`P g(17);` のようなグローバル変数を `main` の前に構築し、`main` の後に破棄する。
- `static const V c = make_v();` のように関数呼び出しで初期化するグローバル変数（動的初期化）。
- 仮想関数を持つクラスのグローバル変数は、仮想関数表へのポインタを静的に初期化する。

**仕組み**

- 構築が必要なグローバル変数ごとに小さな呼び出し関数（`__cpp_gi_N` / `__cpp_gd_N`）を作り、`.init_array` / `.fini_array` に登録する。
- 実行ファイルをリンクするとき、`.init_array` を順に呼ぶ起動関数 `_tcc_cpp_start` を C ソースとして組み込み、エントリポイントを差し替える。破棄は `atexit` で登録する。

**実装箇所**

- `tccgen.c:3026` `cpp_register_global_dyn()` — 構築が必要なグローバル変数の登録
- `tccgen.c:3066` `cpp_register_global_copy_init()`、`tccgen.c:14796` `cpp_emit_global_copy_init_thunk()` — `= 関数呼び出し` 形の登録と構築コード
- `tccgen.c:3463` `cpp_emit_global_dyn_thunk()`、`tccgen.c:3542` `cpp_finish_global_dyns()` — 呼び出し関数の生成と `.init_array` / `.fini_array` への登録
- `tccgen.c:4328` `cpp_init_global_vptr()` — グローバル変数の仮想関数表ポインタの静的初期化
- `tccelf.c:2900` `tcc_add_cpp_init_startup()` — 起動関数 `_tcc_cpp_start` の組み込み
- `tccpe.c:1904-1915` — 実行ファイル生成時にエントリを差し替える判定
- `tcc.h:827` `cpp_global_ctors`、`tcc.h:820` `cpp_init_startup_done`

**amateras での用途**

- `base_inc/mmd/mmd_skinning.h:143` `static const mmd_dq_t c_identity_dq = mmd_dq_make_identity();` — 単位デュアルクォータニオンの定数。メンバ `vec4` がコンストラクタを持つため波括弧で初期化できず、関数の戻り値で初期化している（`dev/test/a9/manual/n7_00_minimal_static_dyninit.cpp` が同じ形を検証）。

**サンプル**

```cpp
// global_ctor.cpp
class P {
public:
    int v;
    P(int n) { v = n; }
};

struct V { int x; };
static V make_v(void) { V v; v.x = 9; return v; }

P g(17);                          // main の前に構築
static const V c_identity = make_v();   // 関数呼び出しで初期化

int main()
{
    return (g.v - 17) + (c_identity.x - 9);
}
```

**制限**

- DLL は対象外。
- `.cpp` を `.o` にしてから別の実行でリンクする形は対象外（同じ実行でコンパイルとリンクを行う）。

### 7.2 関数内 `static` オブジェクト

**できること**

- `static P s;`、`static P s(a, b);`、`static P s = src;` を、最初に到達したときだけ 1 回構築する。
- プログラム終了時にデストラクタを呼ぶ。
- `static M arr[4];` のようなクラス配列も 1 回だけ構築する。

**実装箇所**

- `tccgen.c:2464` `cpp_alloc_local_static_guard()` — 宣言ごとの「初期化済み」フラグ
- `tccgen.c:2484` `cpp_begin_local_static_init()`、`tccgen.c:2492` `cpp_finish_local_static_init()` — フラグを見て 1 回だけ構築する分岐
- `tccgen.c:2803` `cpp_prepare_local_static_dtor()`、`tccgen.c:2834` `cpp_emit_local_static_dtor_registration()` — 終了時デストラクタの登録
- `tccgen.c:5418` `cpp_emit_local_static_array_default_ctor_calls()` — `static` クラス配列の構築
- `tccgen.c:20250-20254` — `decl()` からの接続

**amateras での用途**

- `base_inc/3d_format/nif/nif_scene_impl.h:1664`、`:1700` `static vec3 vertex_storage[32];` — NIF 形式の読み込みで使う作業用配列（`test/tcc/n7_07/test_static_local_vec3_array.cpp` で検証）。

**サンプル**

```cpp
// local_static.cpp
static int ctor_count;

struct Probe {
    int value;
    Probe() { value = 7; ctor_count++; }
    Probe(int a, int b) { value = a + b; ctor_count++; }
};

Probe* get(int a, int b)
{
    static Probe item(a, b);   // 最初の呼び出しの引数で 1 回だけ構築
    return &item;
}

int main()
{
    Probe* p = get(1, 2);
    get(10, 20);               // 2 回目は構築しない
    return (p->value == 3 && ctor_count == 1) ? 0 : 1;
}
```

**制限**

- 初期化の途中で例外が起きた場合の再試行、スレッド安全な初期化は対象外。

### 7.3 `thread_local`

**できること**

- グローバルスコープの `thread_local int value;`、`thread_local int value = 123;`、`thread_local MyClass object;`。
- スレッドごとに別の領域を持ち、最初のアクセス時に構築する。
- スレッド終了時にデストラクタを逆順で呼ぶ。通常の実行ファイルでは PE の TLS コールバック、`-run` では `CreateThread` / `_beginthreadex` / `ExitThread` / `_endthreadex` を包む関数で対応する。

**実装箇所**

- `tcctok.h:172` `TOK_CPP_THREAD_LOCAL`、`tccpp.c:485` — C++ のみのキーワード（C では識別子）
- `tccgen.c:13364-13368` — 記憶クラス `VT_CPP_TLS` の付与（C ではエラー）
- `tcc.h:1158` `VT_CPP_TLS`、`tcc.h:616` `Sym.cpp_tls_desc`
- `tccgen.c:7341` `cpp_alloc_tls_global()`、`tccgen.c:7471` `cpp_push_tls_lvalue()` — TLS 領域の確保とアクセス（毎回 `__tcc_cpp_tls_addr` を呼ぶ）
- `tccgen.c:7393` `cpp_validate_tls_class()` — 対応外の型（仮想関数あり、デフォルトコンストラクタなし等）をエラーにする
- `tccgen.c:7440` `cpp_register_tls_ctor()`、`tccgen.c:7458` `cpp_register_tls_dtor()`、`tccgen.c:3415` `cpp_emit_tls_ctor_thunks()`
- `tccelf.c:1925` `tcc_add_cpp_tls_runtime()` — 実行時コードの組み込み
- `tccpe.c:2057-2061` — `IMAGE_TLS_DIRECTORY` の出力
- `tccrun.c:87`、`165-210`、`384-389` — `-run` 時の対応

**amateras での用途**

- amateras のソースでは C++ の `thread_local` の使用は確認できなかった（amateras のスレッドローカル変数は C のマクロ経由で実装されている）。

**サンプル**

```cpp
// tls_basic.cpp
thread_local int tls_value;

struct Counter {
    int n;
    Counter() : n(100) {}
};
thread_local Counter tls_counter;

int main()
{
    tls_value = 5;
    tls_counter.n += 1;
    return (tls_value == 5 && tls_counter.n == 101) ? 0 : 1;
}
```

**制限**

- 関数内の `static thread_local`、`extern thread_local`、クラスの静的メンバの `thread_local` は未対応（エラーになる）。
- 仮想関数を持つクラス、デフォルトコンストラクタを持たないクラス、クラス配列は未対応（エラーになる）。
- DLL では未対応（エラーになる）。

---

## 8. クラス配列

**できること**

- ローカルのクラス配列 `M a[4];`、多次元配列 `M a[2][3];` の各要素をデフォルトコンストラクタで構築する。
- 関数内 `static` のクラス配列も 1 回だけ構築する。
- クラスのメンバであるクラス配列も自動で構築する。
- デフォルトコンストラクタを持たないクラスの配列はエラーになる。

**実装箇所**

- `tccgen.c:5386` `cpp_emit_local_array_default_ctor_calls()` — ローカル配列の各要素の構築
- `tccgen.c:5343` `cpp_validate_local_automatic_class_array()`、`tccgen.c:5364` `cpp_validate_local_static_class_array()` — 構築できない形の検査
- `tccgen.c:5068` `cpp_emit_member_array_default_ctor_calls()` — メンバ配列の構築
- `tccgen.c:20196`、`20191` — `decl()` からの接続

**amateras での用途**

- `base_inc/town/town_mesh.h:415` `vec2 dir, nrm, corners[4];`、`:494` `vec2 corners[4];`、`base_inc/town/town_town.h:702` `vec2 to_relax[4];` — 街生成のメッシュ計算で使うローカル配列（`test/tcc/n7_07/test_town_mesh_406.cpp` などで検証）。

**サンプル**

```cpp
// class_array.cpp
static int ctor_count;

struct M {
    int v;
    M() : v(7) { ctor_count++; }
};

int main()
{
    M a[4];
    M b[2][3];
    return (a[3].v == 7 && b[1][2].v == 7 && ctor_count == 10) ? 0 : 1;
}
```

**制限**

- 要素を明示初期化する形 `M a[2] = { ... }` は未対応。
- デストラクタを持つクラスの配列は未対応（エラーになる）。
- `thread_local` のクラス配列は未対応（エラーになる）。

---

## 9. 代入演算子と暗黙のコピー代入

**できること**

- ユーザー定義の `operator=`（メンバ）。`T&` を返せば `a = b = c` と連鎖できる。
- `operator=` を書いていないクラスの `a = b`。
  - メンバや基底クラスが `operator=` を持つ場合は、メンバごとにそれぞれの `operator=` を呼ぶ。
  - どのメンバも `operator=` を持たない場合は、バイト列をコピーする（従来どおり）。
- 参照メンバや `const` メンバを持つクラス、仮想関数を持つクラスの暗黙コピー代入はエラーになる。

**実装箇所**

- `tccgen.c:12267` `cpp_try_member_binop()` — ユーザー定義 `operator=` の呼び出し
- `tccgen.c:12029` `cpp_implicit_copy_assign_is_safe()` — バイト列コピーで済ませてよいかの判定
- `tccgen.c:12063` `cpp_implicit_copy_assign_is_memberwise_viable()` — メンバごとの代入が可能かの判定
- `tccgen.c:12230` `cpp_emit_implicit_memberwise_copy_assign()` — メンバごとの代入コードの生成
- `tccgen.c:16958-16983` — `expr_eq()` の `=` の処理からの接続

**amateras での用途**

- `base_inc/vec_quat.h:66` `vec2 operator=(const vec2&)`、`:170` `vec3 operator=(const vec3&)`、`:293` `vec4& operator=(const vec4&)`、`:303` `vec4& operator=(const vec3&)`。
- `base_inc/utility/txt_util.h:68` `win_txt &operator=(const win_txt &o)` — 自己代入チェック付きの深いコピー。
- `base_inc/mmd/vmd_parse.h:44` `mmd_vmd_bone_key_t` の代入 `tr.keys[n] = key;` — `operator=` を持たない構造体だが、メンバ `vec3` / `vec4` の `operator=` が呼ばれる（`dev/test/a9/manual/n7_03_amateras_bone_key.cpp`）。

**サンプル**

```cpp
// copy_assign.cpp
struct M {
    int v;
    M() { v = 1; }
    M& operator=(const M& o) { v = o.v + 1000; return *this; }
};
struct H {          // operator= を書いていない
    M m;
    int n;
    H() { n = 0; }
};

int main()
{
    H x, y;
    x.m.v = 42;
    y = x;          // y.m = x.m は M::operator= を経由する
    return y.m.v == 1042 ? 0 : 1;
}
```

---

## 10. 継承

### 10.1 単一継承・多重継承

**できること**

- `class D : public B` の単一継承。派生から基底のメンバ変数・メンバ関数へアクセスできる。
- `class D : public A, public B` の多重継承（仮想継承なし）。2 番目以降の基底のメンバ関数を呼ぶときは `this` を調整する。
- 派生クラスのコンストラクタから基底の初期化 `: Base(args)`。書かなければ基底のデフォルトコンストラクタを自動で呼ぶ。
- 派生クラスのデストラクタの後に基底のデストラクタを自動で呼ぶ。
- `D*` → `B*`、`D&` → `B&` の変換（アドレス調整を含む）。
- 基底クラスの実装を明示的に呼ぶ `Base::method(args)`。
- `sizeof(D)` は基底と派生の合計。

**実装箇所**

- `tccgen.c:12695-12707` — 基底クラスの解析と、基底を先頭（または直後）に埋め込む
- `tccgen.c:11389` `cpp_base_subobject_offset()` — 基底オブジェクトの位置（曖昧なら負を返す）
- `tccgen.c:4562` `cpp_emit_base_ctor_call()` — `: Base(args)` の呼び出し
- `tccgen.c:4792` `cpp_emit_implicit_base_ctors()` — 書かれていない基底のデフォルト構築
- `tccgen.c:4824` `cpp_emit_base_dtor_calls()` — 基底の自動破棄
- `tccgen.c:4540` `cpp_find_base_field()` — 基底の埋め込みフィールドの検索
- `find_field()` の再帰 — 派生から基底メンバへのアクセス

**amateras での用途**

- `base_inc/render/opengl/render_opengl.h:7` `struct render_opengl : public window_t` — OpenGL バックエンドはウィンドウ型を継承し、`init_opengl()`、`begin_frame()`、`end_frame()` をメンバ関数として持つ。C++ からは `render_opengl` のオブジェクト経由で使う（`amateras対応作業履歴.md` §2.6）。
- `base_inc/render/dx/9/win_dx9.h:28` `class dx9_render : public window_t` など DirectX 系、`base_inc/render/agc/triangle_agc.h:42` `class triangle_agc : public agc_render` — 定義は存在するが、TCC での検証が済んでいるのは OpenGL 経路のみ。

**サンプル**

```cpp
// inherit.cpp
class Base {
public:
    int b;
    Base() : b(1) {}
    int get() { return b; }
};
class Other {
public:
    int o;
    Other() : o(2) {}
    int geto() { return o; }
};
class Derived : public Base, public Other {
public:
    int d;
    Derived(int x) : Base(), d(x) {}   // Other は自動でデフォルト構築
};

int main()
{
    Derived v(3);
    Base* pb = &v;                     // 先頭の基底へ
    Other* po = (Other*)&v;            // 2 番目の基底へ（アドレス調整される）
    return v.get() + v.geto() + v.d + pb->b + po->o - 9;   // 1+2+3+1+2
}
```

**制限**

- 仮想継承（`virtual public`）、菱形継承は未対応（エラーになる）。
- 基底ポインタから派生ポインタへの逆方向変換ではアドレス調整をしない。
- 多重継承時に基底クラスのメンバ関数ポインタを使う形は未対応。

### 10.2 仮想関数

**できること**

- `virtual` メンバ関数、派生クラスでの上書き、値・ポインタ・参照経由の動的な呼び分け。
- 純粋仮想関数 `= 0` と抽象クラス。抽象クラスのオブジェクト宣言と `new` はエラーになる（ポインタ・参照は可）。
- 仮想デストラクタ。`delete base_ptr` で派生のデストラクタから順に呼ぶ。基底が仮想なら派生のデストラクタも自動で仮想になる。
- 多重継承で 2 番目以降の基底にも仮想関数がある場合、基底ごとの仮想関数表と、`this` を調整してから本体へ飛ぶ小さな中継関数を生成する。
- 仮想関数表は弱シンボルとして出力するため、複数のソースファイルで同じクラスを定義してもリンクできる。

**実装箇所**

- `tccgen.c:3758` `cpp_assign_virtual_slots()` — 仮想関数表の並びを決める
- `tccgen.c:3805` `cpp_insert_vptr_field()` — オブジェクト先頭に仮想関数表ポインタを入れる
- `tccgen.c:3931` `cpp_emit_vtable()` — 仮想関数表（`__cpp_vtbl_<Class>`）の出力。表の 1 つ前の要素にオブジェクト先頭までの距離を置く
- `tccgen.c:4071` `cpp_emit_secondary_vtables()`、`tccgen.c:4019` `cpp_new_virtual_thunk()`、`tccgen.c:4203` `cpp_finish_virtual_thunks()` — 多重継承用の 2 番目以降の表と中継関数
- `tccgen.c:4289` `cpp_init_local_vptr()`、`tccgen.c:4328` `cpp_init_global_vptr()`、`tccgen.c:14536` `cpp_init_heap_vptr()` — ローカル / グローバル / `new` のオブジェクトの仮想関数表ポインタの初期化
- `tccgen.c:4357` `cpp_prepare_virtual_member_call()` — 仮想関数表を経由した呼び出し
- `tccgen.c:3900` `cpp_class_is_abstract()`、`tccgen.c:3918` `cpp_check_not_abstract()` — 抽象クラスの判定
- `tcc.h:546` `FuncAttr.func_pure` — 純粋仮想の印
- `tccgen.c:3743` `cpp_find_virtual_dtor_in_chain()` — 仮想デストラクタの継承

**amateras での用途**

- `base_inc/render/agc/agc_define.h:49-53` `virtual ~agc_render()`、`virtual bool init()`、`virtual void render()` など、`base_inc/render/dx/9/win_dx9.h:110` `virtual void render()` — レンダリングバックエンドの共通インタフェース。定義は存在するが、TCC で検証済みなのは OpenGL 経路であり、これらの仮想関数を TCC で実行した記録は無い。

**サンプル**

```cpp
// virtual_basic.cpp
static int state;

class Shape {
public:
    virtual int area() = 0;          // 純粋仮想
    virtual ~Shape() { state = state * 10 + 1; }
};
class Square : public Shape {
public:
    int s;
    Square(int n) : s(n) {}
    virtual int area() { return s * s; }
    ~Square() { state = state * 10 + 2; }   // 基底が仮想なので自動で仮想
};

int main()
{
    Shape* p = (Shape*)new Square(3);
    int a = p->area();               // 9
    delete p;                        // ~Square → ~Shape の順（state == 21）
    return (a == 9 && state == 21) ? 0 : 1;
}
```

**制限**

- `dynamic_cast`、`typeid` は未対応。
- 上書きされていない純粋仮想関数のスロットは空（NULL）のまま。抽象クラスのオブジェクトを作れないため到達しない。
- 深い継承で孫クラスが祖父母の 2 番目の基底の仮想関数を新たに上書きする形はエラーになる。

---

## 11. `new` / `delete`

**できること**

- `new Class(args)`、`new Class()`、`new Class` — メモリ確保 → 仮想関数表ポインタの初期化 → コンストラクタ（オーバーロード解決・デフォルト引数込み）。
- `delete p` — NULL のときは何もしない → デストラクタ（仮想なら動的に呼び分け） → 解放。
- `new T(obj)` — コピーコンストラクタ、または メンバごとのコピーで構築。
- コンストラクタもデストラクタも持たない型の `new T[n]` / `delete[]`。
- 入れ子の `new`、`delete 0`。

**実装箇所**

- `tccgen.c:14977` `cpp_parse_new()`、`tccgen.c:15053` `cpp_parse_delete()` — 構文解析と生成
- `tccgen.c:15778-15784` — `unary()` からの入口
- `tccgen.c:14834` `cpp_emit_heap_ctor_call()` — 確保した領域へのコンストラクタ呼び出し
- `tccgen.c:14536` `cpp_init_heap_vptr()` — 仮想関数表ポインタの初期化

**amateras での用途**

- amateras の本体（`base_inc/**`）は C でもビルドできるように書かれており、`new` / `delete` は `base_inc/render/nvn/switch_nvn_context.h:885` `delete s;`（Switch 向け、TCC の対象外）と `base_inc/utility/leakdetect.h`（`operator new` の置き換え、TCC 未対応）にしか無い。
- `base_inc/cross_base.h:705` のコメント「C++: w はユーザー所有。delete でデストラクタ(~win_txt)が buf を解放」— 利用側コードで `delete` を使う想定。

**サンプル**

```cpp
// new_delete.cpp
static int g_dtor;

class P {
public:
    int v;
    P() { v = 5; }
    P(int a, int b) { v = a * b; }
    ~P() { g_dtor++; }
};

int main()
{
    P* a = new P();
    P* b = new P(3, 4);
    int r = a->v + b->v;   // 5 + 12
    delete a;
    delete b;
    int* arr = new int[4];   // コンストラクタを持たない型の配列
    arr[3] = 1;
    delete[] arr;
    return (r == 17 && g_dtor == 2) ? 0 : 1;
}
```

**制限**

- `new int` のような単一のスカラ型の `new` は未対応（エラーになる）。配列形 `new int[n]` は可。
- コンストラクタまたはデストラクタを持つクラスの `new C[n]` / `delete[]` は未対応（エラーになる）。
- `operator new` の置き換え、配置 `new` は未対応。

---

## 12. 演算子オーバーロード

**できること**

| 種類 | 演算子 | メンバ | 非メンバ |
|---|---|---|---|
| 二項算術 | `+ - * / % & \| ^ << >>` | 可 | 可 |
| 比較 | `== != < > <= >=` | 可 | 可 |
| 代入 | `=` | 可 | — |
| 複合代入 | `+= -= *= /= %= &= \|= ^= <<= >>=` | 可 | 可 |
| 単項 | `! - ~` | 可 | 可 |
| 前置 | `++ --` | 可 | 可 |
| 後置 | `++ --`（`operator++(int)`） | 可 | 可 |
| 添字 | `[]` | 可 | 可（本プロジェクト独自。標準 C++ では不可） |
| 間接参照 | `*`（単項） | 可 | 可 |
| アロー | `->` | 可（1 段だけ） | — |

- 明示呼び出し `a.operator+(b)` も可。
- 構造体を返す演算子、8 バイトを超える構造体を返す演算子も可。
- `T& operator*()` のように参照を返せば、`(*it).v = 5` のように左辺値として使える。
- `s << a << b` のような連鎖も可。

**実装箇所**

- `tccgen.c:1137` `cpp_operator_suffix()`、`tccgen.c:1189` `cpp_operator_field_tok()` — 演算子ごとの内部名（`__cpp_op_plus` など）
- `tccgen.c:1222` `cpp_parse_operator_decl_name()`、`tccgen.c:13970-13979` — `operator+` 宣言の解析
- `tccgen.c:12267` `cpp_try_member_binop()` — メンバの二項演算子（引数の型でオーバーロードを採点する）
- `tccgen.c:11737` `cpp_try_free_binop()` — 非メンバの二項演算子
- `tccgen.c:11781` `cpp_try_cpp_subscript()` — `[]`
- `tccgen.c:11844` `cpp_try_member_unop()`、`tccgen.c:11865` `cpp_try_free_unop()` — 単項と前置
- `tccgen.c:11904` `cpp_try_member_postop()`、`tccgen.c:11932` `cpp_try_free_postop()` — 後置
- `tccgen.c:10793` `cpp_find_operator_member()` — メンバ演算子の検索
- `tccgen.c:16013` — `->` の入口（`operator->` を 1 段だけ適用する）
- `tccgen.c:11501` `cpp_finish_member_call()`、`tccgen.c:11630` `cpp_finish_free_call()` — 実際の呼び出しコード

**amateras での用途**

- `base_inc/vec_quat.h` — `vec2` / `vec3` / `vec4` の `operator= * + - / *=`、単項 `-`（`:243`）、`float operator[](int i)`（`:119`）、`arr2` / `arr3` の `operator== + +=`（`:505-523`）。ベクトルと行列の計算はほぼすべてこの演算子経由で書かれている。
- `test/tcc/vec_quat/vec_quat_matrix_cpp.cpp:46-56` — `a * 2.0f`、`a + c`、`e *= 3.0f`、`q * 0.5f` を実行して値を検証している。
- 同名演算子 `operator*(vec3&)` と `operator*(float)` は宣言順に関係なく引数の型で選ばれる（以前は宣言順で決まる不具合があり、`dev/test/a9/manual/AMATERAS_TO_TCC_BLOCKER_N7_02_RETEST.md` で修正した）。

**サンプル**

```cpp
// operators.cpp
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float a, float b) : x(a), y(b) {}
    Vec2 operator+(const Vec2& o) { Vec2 r(x + o.x, y + o.y); return r; }
    Vec2 operator*(float s)       { Vec2 r(x * s, y * s); return r; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    bool operator==(const Vec2& o) { return x == o.x && y == o.y; }
    float operator[](int i)       { return i == 0 ? x : y; }
    Vec2 operator-()              { Vec2 r(-x, -y); return r; }
};

int main()
{
    Vec2 a(1.0f, 2.0f);
    Vec2 b = a * 2.0f;          // (2, 4)
    Vec2 c = a + b;             // (3, 6)
    c += a;                     // (4, 8)
    Vec2 d = -a;                // (-1, -2)
    Vec2 e(4.0f, 8.0f);
    if (!(c == e)) return 1;
    if (c[1] != 8.0f) return 2;
    if (d.x != -1.0f) return 3;
    return 0;
}
```

**制限**

- `operator new` / `operator delete` の置き換えは未対応。
- `->` は 1 段だけ適用し、ポインタ以外を返すとエラーになる。
- 型変換演算子 `operator int()` は未対応。

---

## 13. メンバポインタ

**できること**

- データメンバポインタ `int Point::* pm = &Point::x;`、`obj.*pm`、`ptr->*pm` の読み書き。
- メンバ関数ポインタ `void (Point::*pf)(int) = &Point::set;`、`(obj.*pf)(42)`。
- 仮想関数へのメンバ関数ポインタは、オブジェクトの実際の型で呼び分ける。
- 派生クラスのオブジェクトに基底クラスのメンバ関数ポインタを使える（単一継承のみ）。

**実装箇所**

- `tcc.h:1156` `VT_MPTR` — メンバポインタ型のフラグ
- `tccgen.c:1438` `cpp_parse_member_pointer()` — `T Class::*` の宣言解析
- `tccgen.c:1475` `cpp_parse_qualified_member()` — `&Class::member` の解析
- `tccgen.c:1526` `cpp_emit_mptr_dmp_access()` — データメンバポインタのアクセス
- `tccgen.c:1555` `cpp_emit_mptr_pmf_invoke()` — メンバ関数ポインタの呼び出し（仮想なら `cpp_prepare_virtual_member_call()` へ）
- `tccgen.c:16036-16038` — `.*` / `->*` の入口

**amateras での用途**

- amateras のソースではメンバポインタの使用は確認できなかった。

**サンプル**

```cpp
// member_pointer.cpp
class Point {
public:
    int x;
    void set(int v);
    int get();
};
void Point::set(int v) { x = v; }
int Point::get() { return x; }

int main()
{
    void (Point::*pset)(int) = &Point::set;
    int (Point::*pget)() = &Point::get;
    int Point::*px = &Point::x;
    Point p;
    (p.*pset)(42);
    p.*px = (p.*px) + 1;
    return (p.*pget)() - 43;
}
```

**制限**

- 多重継承のメンバ関数ポインタは未対応。
- MSVC の 16 バイトのメンバ関数ポインタ形式とは互換性がない。

---

## 14. 名前解決

### 14.1 スコープ修飾 `::`

**できること**

- 先頭の `::` によるグローバルスコープ指定（型位置 `typedef ::C D;`、式位置 `::gfn()` / `::gv`）。ローカル変数が同名を隠していてもグローバルを選ぶ。
- クラス内 typedef（`typedef unsigned int size_type;`）と、その修飾参照 `Class::size_type`。
- 二重修飾のクラス外定義 `Outer::Inner::method()`。
- 三項演算子との隣接 `a ? b : ::c`。

**実装箇所**

- `tccgen.c:794` `cpp_parse_global_scope_qualifier()` — 先頭 `::`
- `tccgen.c:6792` `cpp_global_scope_find()`、`tccgen.c:6819` `cpp_global_lookup_type_name()` — グローバル限定の探索
- `tccgen.c:2912` `cpp_class_typedef_find()`、`tcc.h:621` `Sym.cpp_class_typedefs` — クラス内 typedef（メンバの並びとは別に保持する）
- `tccgen.c:893` `parse_cpp_scope_qualifier()` — 多段の `A::B::` の解析

**amateras での用途**

- amateras のソースでは `::` の使用は確認できなかった（CPPUnit の移植で使用）。

**サンプル**

```cpp
// scope_qualifier.cpp
int gv = 7;
class S {
public:
    typedef unsigned int size_type;
    static const size_type npos;
    size_type len;
};
const S::size_type S::npos = 100;

int main()
{
    int gv = 1;                 // グローバルの gv を隠す
    S::size_type n = S::npos;   // 100
    return ::gv + (int)n - 107; // 7 + 100
}
```

**制限**

- `namespace` と `using` は未対応。

### 14.2 名前の隠蔽

**できること**

- 引数やローカル変数がクラス名と同じでも、その変数を優先する（C++ の規則どおり）。
- 内側の typedef が外側の同名クラスを隠す。
- メンバ関数内では、自クラスのメンバが同名のグローバル関数より優先される。
- クラス本体内の型名は、ブロックローカル → 自クラスと直接基底 → 外側クラス → グローバルの順に探す。

**実装箇所**

- `tccgen.c:2886` `cpp_lookup_type_name()` — C++ の規則に従った型名の探索（共通ヘルパー）
- `tccgen.c:2987` `cpp_tok_starts_type_name()` — 文頭の識別子が型名かどうかの判定
- `tccgen.c:2968` `cpp_unqualified_class_type_find()` — 非修飾のクラス型の探索

**amateras での用途**

- `base_inc/vec_quat.h:406` `struct tex`（C++ のときだけ `struct tex`、C では `typedef struct tex`）と、`inc/UTF_8/cross.h` の `mmd_gl_free_texture()` の引数 `tex` — 引数がクラス名を隠す形。この処理が無いと `tex->gl_tex_id = 0;` がコンパイルできなかった（`amateras対応作業履歴.md` §2.2、回帰テスト `dev/test/a9/bug20_shadow_param.cpp`）。
- `win_txt::clear()` と `window_t::clear[4]` — 別クラスのメンバ関数名と自クラスのメンバ変数名が同じ形（`amateras対応作業履歴.md` §2.6、回帰テスト `dev/test/a9/bug21_member_vs_global.cpp`）。

**サンプル**

```cpp
// name_hiding.cpp
struct tex { float u, v; };          // クラス名 tex
struct T { unsigned int id; };

static void free_tex(T* tex)          // 引数名 tex がクラス名 tex を隠す
{
    tex->id = 0;                      // 型名ではなく引数として扱われる
}

int main()
{
    T t;
    t.id = 1;
    free_tex(&t);
    return t.id;                      // 0
}
```

**制限**

- 同じグローバルスコープで `struct tex {...}; int tex;` の順に書くと再定義エラーになる（逆順や関数内なら可）。
- グローバル変数の名前を、どこかのクラスのメンバ関数名と同じにはできない。

---

## 15. 関数形式のキャストと一時オブジェクト

**できること**

- `size_type(-1)`、`int('A')` のような基本型・typedef 名の関数形式キャスト。
- `Class::size_type(-1)` の修飾形。
- コンストラクタを持つクラスの一時オブジェクト `Foo(1)`、`Vec3(a, b, c)`。
- 一時オブジェクトは文の終わりで自動的に破棄される。

**実装箇所**

- `tccgen.c:14336` `cpp_try_functional_cast()` — 式の先頭が型名で直後が `(` のときの処理
- `tccgen.c:14281` `cpp_tok_is_cast_type_name()` — 型名かどうかの判定（推測ではなく型の探索で決める）
- `tccgen.c:11266` `cpp_functional_ctor_temp()` — クラス型の一時オブジェクトの構築
- `tccgen.c:1893` `cpp_note_class_temp()` — 一時オブジェクトの破棄予約

**amateras での用途**

- `base_inc/vec_quat.h:46-48` `#define VEC3_LIT(x, y, z) vec3((x), (y), (z))` — C++ のときはリテラル初期化子を一時オブジェクトに展開する。`base_inc/mmd/mmd_bone.h:314-315`、`base_inc/mmd/mmd_anim.h:514-515`、`base_inc/mmd/mmd_skinning.h:139-140` で使用。

**サンプル**

```cpp
// functional_cast.cpp
typedef unsigned int size_type;

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float a, float b, float c) : x(a), y(b), z(c) {}
};
#define VEC3_LIT(x, y, z) Vec3((x), (y), (z))

int main()
{
    size_type a;
    a = size_type(-1);
    Vec3 v = VEC3_LIT(1.0f, 2.0f, 3.0f);   // 一時オブジェクトからのコピー初期化
    if (a != 0xffffffffu) return 1;
    if (int('A') != 65) return 2;
    return (v.z == 3.0f) ? 0 : 3;
}
```

**制限**

- コンストラクタを持たないクラスの `Foo(1)`、引数なしの `Foo()` によるゼロ初期化は未対応（エラーになる）。
- `static_cast` などのキャスト演算子は未対応。

---

## 16. 未対応の機能

以下は現在の実装で使えない。多くはコンパイルエラーとして検出される。

| 機能 | 状態 |
|---|---|
| テンプレート（関数・クラス） | 未対応。エラーになる |
| 名前空間 `namespace` / `using` | 未対応。エラーになる |
| 例外 `try` / `catch` / `throw` | 未対応。エラーになる |
| キャスト演算子 `static_cast` など | 未対応。エラーになる |
| 実行時型情報 `dynamic_cast` / `typeid` | 未対応 |
| 標準ライブラリ `<iostream>` など | ヘッダが無い |
| 仮想継承・菱形継承 | 未対応。エラーになる |
| `explicit`、`mutable`、クラス内 `enum` | 未対応。エラーになる |
| C++11 以降の機能（`auto`、ラムダ、`constexpr`、`nullptr`、メンバ初期化子、`enum class`、右辺値参照） | 未対応 |
| `operator new` の置き換え、配置 `new` | 未対応 |
| デストラクタを持つクラスの配列、`new C[n]` | 未対応。エラーになる |
| 派生クラスの値を基底クラスの値へコピーする形（値渡し・値返し・コピー初期化） | エラーになる（参照または直接初期化を使う） |
| DLL でのグローバルオブジェクト構築 / `thread_local` | 未対応 |
| DLL でのグローバルオブジェクト構築 / `thread_local` | 未対応 |

**標準からの逸脱（エラーにならないもの）**

| 事項 | 挙動 |
|---|---|
| 未宣言関数の呼び出し | C++ 標準ではエラーだが、tpp は TCC 由来の C 挙動を引き継ぎ、`implicit declaration of function '<名前>'` の警告を出して `int` を返す無型関数として扱う。メンバ関数の本体からでも自由関数からでも同じ（`dev/test/a9/member_body_implicit_decl.cpp`）。`<stdio.h>` を取り込まずに `printf` を呼ぶような書き方が通ってしまうので、C++ として書くなら宣言を自分で用意すること |

---

## 17. amateras からの逆引き

amateras のどの部分が、どの C++ 機能に依存しているかをまとめる。

| amateras の機能 | ファイル | 使っている C++ 機能 |
|---|---|---|
| ベクトル・クォータニオン・行列 | `base_inc/vec_quat.h` | クラス、コンストラクタ、参照引数、演算子オーバーロード（`= * + - / *= [] ==` 単項 `-`）、オーバーロード解決、`bool`、関数形式キャスト（`VEC3_LIT`）、名前の隠蔽（`struct tex`） |
| ウィンドウ管理 | `base_inc/cross_base.h`、`base_inc/platform/win/win_imp.h` | メンバ関数（`init()`、`reg_win()`、`run()`）、`this`、コンストラクタ / デストラクタ、クラス型メンバ（`win_txt`）の自動構築・破棄 |
| 文字列バッファ | `base_inc/utility/txt_util.h` | コンストラクタのオーバーロード、コピーコンストラクタ、`operator=`、デストラクタ、参照 |
| OpenGL バックエンド | `base_inc/render/opengl/render_opengl.h` | 単一継承（`render_opengl : public window_t`）、メンバ関数 |
| MMD スキニング | `base_inc/mmd/mmd_skinning.h` | 関数呼び出しによるグローバル定数の動的初期化、クラス型メンバの自動構築 |
| MMD ボーン / アニメーション | `base_inc/mmd/vmd_parse.h`、`base_inc/mmd/mmd_bone.h`、`base_inc/mmd/mmd_anim.h` | クラス型メンバの自動構築、暗黙のメンバごとコピー代入、関数形式キャスト |
| 街生成（メッシュ） | `base_inc/town/town_mesh.h`、`base_inc/town/town_town.h` | ローカルのクラス配列 |
| NIF 形式の読み込み | `base_inc/3d_format/nif/nif_scene_impl.h` | 関数内 `static` のクラス配列 |
| C の API との接続 | `base_inc/cross_define.h`、`base_inc/3d_format/**/*.h` | `extern "C"` |
| C++ からの動作確認 | `test/tcc/vec_quat/vec_quat_matrix_cpp.cpp`、`test/tcc/n7_06/test_mmd_cross_cpp_compile.cpp`、`test/tcc/n7_07/*.cpp` | 上記全体の統合確認（`cross.h` を C++ として取り込み、MMD モデルの描画まで実行） |

amateras 側の確認結果（`test/tcc/n7_08/AMATERAS_N7_08_FINAL_CONSUMER_QUALIFICATION.md`、2026-09-12）では、`cross.h` の C++ コンパイル、MMD + OpenGL の C++ ビルド・リンク・実行がすべて成功している。

---

## 18. 主要な回帰テストの場所

| ディレクトリ | 内容 |
|---|---|
| `dev/test/a2/` | C ソースでのキーワード降格 |
| `dev/test/a7/` | メンバ関数、デフォルト引数、typedef |
| `dev/test/a8/` | 継承、静的メンバ、コンストラクタ、デストラクタ、仮想関数、メンバポインタ |
| `dev/test/a9/` | 演算子、`const` メンバ関数、参照、コピー初期化、`new` / `delete`、多重継承、名前解決、バグ回帰 |
| `dev/test/a9/negative/` | エラーになるべき形（`.expected` に期待する診断） |
| `dev/test/a9/manual/` | `thread_local`、クラス配列、amateras と同じ形の再現テスト |
| `sample/cppunit/` | CPPUnit の移植（12 ファイル + テストドライバ、17 テスト成功） |
| `dev/test/run_all.bat` | 上記をまとめて実行する仕組み（`build.bat` から呼ばれる） |
