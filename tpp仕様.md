# tpp C++ 仕様書

**対象**: `dev\tcc.exe` — tcc version 0.9.28rc (x86_64 Windows)
**最終確認**: 2026-08-29（`build.bat` フルゲート緑 — Release|x64 / Debug|Win32、CUnit 14/14、
`run_all.bat` 0 gating failure / 0 crash、CPPUnit G7 機械ゲート `TESTS:17 FAILURES:0 ERRORS:0`）
**位置づけ**: TCC に **C++98 のサブセット**を実装したもの。C++ コンパイラの代替ではない。

関連文書: [実装済み.md](実装済み.md)（機能一覧の正本） / [問題と原因.md](問題と原因.md)（バグ事例） /
[IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)（実装プラン） / [amateras対応作業履歴.md](amateras対応作業履歴.md)

> 本書の「動作しない」節は、すべて **`dev\tcc.exe` で実測**した結果である。
> 推測は含まない。判定は「ビルド NG」（コンパイルエラー）と
> 「**ビルド OK・動作誤り**」（黙って通るが結果が違う = 最も危険）を区別して記載する。
>
> 基準となる実測は 2026-08-09。以降、ガイド G1〜G7（2026-08-22〜08-25）で解消した
> 項目は取り消し線と対応ガイド／BUG 番号を添えて更新している。2026-08-29 に
> `build.bat` フルゲートを再実行し 0 failure / 0 crash・CPPUnit 17/17 を再確認した。
> §3.2 A（コピーコンストラクタ）は 2026-08-29 に最小再現で再実測し、**未解消**である
> ことを確認済み（`P a; P b = a;` で `b.v` がコピー ctor を通らない）。

---

## 1. 基本仕様

### 1.1 C++ モードに入る条件

| 条件 | 内容 |
|---|---|
| 拡張子 | `.cpp` / `.cxx` / `.cc` / `.hpp` |
| 明示指定 | `-x c++` / `-x c++-header` |
| `.h` | **親 TU のモードを継承**（`.h` 単体では C 扱い） |
| `.c` | 常に C。`class` 等の C++ キーワードは識別子へ降格される |

混合コンパイル（`tcc foo.cpp bar.c`）は可。`.c` 側は C として処理される。

### 1.2 定義済みマクロ

```
__cplusplus = 199711   (C++98。C++ TU のみ定義)
__TINYC__              (C/C++ 共通)
```

SDK ヘッダを TCC 互換にする分岐は `#if defined(__cplusplus) && !defined(__TINYC__)` の形で書く。

### 1.3 名前修飾（マングリング）

TCC 独自形式で、Itanium / MSVC とは**互換性がない**。

```
自由関数      __tcc_<name>_<args>
メンバ関数    __tcc_<Class>__<name>_<args>
コンストラクタ __cpp_ctor_<Class>
デストラクタ  __cpp_dtor_<Class>
vtable        __cpp_vtbl_<Class> / __cpp_vtbl2_<D>_<B>
this 調整     __cpp_vthunk_*
```

`main` / `wmain` はマングルしない。`extern "C"` を付けた宣言もマングルしない。

**帰結**: 他のコンパイラが作った C++ オブジェクトとはリンクできない。
C++ で外部ライブラリと繋ぐ場合は `extern "C"` インタフェース経由に限られる。

---

## 2. 実装済みの C++ 機能

詳細な一覧と対応コミットは [実装済み.md](実装済み.md) が正本。ここでは要約する。

| 分類 | 動作するもの |
|---|---|
| クラス | `class` / `struct`、アクセス指定子（パース）、メンバ変数・メンバ関数、クラス内インライン定義、クラス外定義（`int Foo::bar()`）、**クラス内 typedef**（unqualified / `Class::type` 修飾 / 基底・外側クラスからの継承解決、G3）、二重修飾クラス外定義 `Outer::Inner::member`、デフォルト引数の定義スコープ解決（`= npos`）（いずれも 2026-08-22）、ネストクラスのクラス外定義 `class Outer::Inner { ... };`、**局所クラス**（関数内のクラス定義。BUG-42。制限: タグ名が関数終了後もファイルスコープに残るため同名局所クラスの複数定義は不可）（2026-08-23） |
| 型 | `bool` / `true` / `false`、参照 `&`（引数・戻り値・ローカル・**メンバ**）、クラス名 typedef |
| 関数 | オーバーロード、デフォルト引数、無名引数、`const` メンバ関数、`const` によるオーバーロード |
| 構築・破棄 | コンストラクタ（初期化子リスト含む）、デストラクタ、ローカルのブロック終了時 dtor、グローバルの自動 ctor/dtor（`.init_array` / `.fini_array`）、暗黙の基底 ctor / 自動基底 dtor、**`const` メンバの mem-initializer 初期化**、**暗黙メンバ ctor/dtor**（mem-init が挙げないクラス型データメンバの黙示構築・逆順破棄）、**変換 ctor の暗黙適用**（G-CONV。`return` / mem-init / 代入 / 引数で 1 段のみ）（いずれも 2026-08-23）、**コピー初期化 `T b = a;`**（FEAT-COPY-INIT、2026-08-29。ユーザー定義コピー ctor を解決、無ければメンバワイズ再構築。直接初期化 `T b(a);` と同じ意味論。参照・deref・関数戻り値からの初期化も可。**派生 → 基底のスライシングだけは明示エラー** — BUG-49 の多重適用を避けるため） |
| 動的確保 | `new Class(args)` / `new Class()` / `new Class` / `delete p`（G4、2026-08-22。vptr 初期化 → ctor のオーバーロード解決・デフォルト引数込み）、`new POD[n]` / `delete[]`（POD のみ）、**`new T(obj)` の暗黙メンバワイズコピー**（BUG-46/47、2026-08-24。ヒープバッファを持つメンバはコピー用 ctor で再構築）。**明示エラー**: scalar POD の `new int`、ctor/dtor を持つクラスの `new C[n]` / `delete[]`、`operator new` の置き換え |
| 継承 | 単一継承、多重継承（非仮想）、`D*`→`B*` / `D&`→`B&` アップキャスト |
| 仮想関数 | `virtual` 宣言、vtable / vptr、動的ディスパッチ（値・ポインタ経由）、派生 override、多重継承下の仮想（primary vptr 共有 + セカンダリ vtable + this 調整 thunk）、**純粋仮想 `= 0` / 抽象クラス**（G5、2026-08-22。抽象クラスのオブジェクト宣言・`new` は明示エラー）、**仮想デストラクタ + complete-object delete**（G6、2026-08-22。`delete base_ptr` の動的ディスパッチ、MI は offset-to-top で complete object を free） |
| メンバポインタ | データメンバポインタ `T C::*`、非仮想 / 仮想メンバ関数ポインタ |
| 演算子 | `+ - * / % & \| ^ << >>`、比較 `== != < > <= >=`、代入 `= += -= *= /=` と各種複合代入、単項 `! - ~`、前置・後置 `++ --`、`[]`、**単項 `operator*`（deref）/ `operator->`**（G-OP、2026-08-22。`->` は 1 段適用・非ポインタ返却はエラー、メンバのみ）、メンバ / 非メンバの両方 |
| リンケージ | `extern "C" { ... }` ブロック、`extern "C" void f();` 単一宣言 |
| その他 | `this` / `*this` / `this->x`、`Class::member` / `Class::func()`、ネストしたクラス定義、**修飾基底メンバ呼び出し** `Base::method(args)`（非仮想の直接バインド + this の基底調整。2026-08-23）、関数形式キャスト `T(expr)`（typedef・基本型・`Class::type`。G-CAST、2026-08-22。**ctor を持つクラス型の一時オブジェクトも対応** — 1 引数 `Foo(1)` は 2026-08-23、多引数 `T(a1,...,an)` は G-FCAST-N で 2026-08-23。ctor なしクラス / `T()` value-init は明示エラー）、`friend class X;`（受理して読み捨て。アクセス制御は元々未実装のため意味差なし。friend **関数**宣言は明示エラー。G2、2026-08-22）、先頭 `::`（グローバルスコープ修飾。型位置 `typedef ::C D;` / 式位置 `::gfn()`。**正式 lookup 実装** — ローカルや同名メンバが隠していてもグローバル束縛へ解決し、shadow テストは exit 0。G1、2026-08-22） |

---

## 3. 動作しない C++ 機能（実測）

### 3.1 ビルドが通らない（コンパイルエラー）

| 機能 | 最小再現 | tcc の診断 |
|---|---|---|
| **テンプレート**（関数・クラスとも） | `template<class T> T mx(T a,T b){...}` | `';' が必要です（"<" が見つかりました）` |
| **名前空間** `namespace` | `namespace N { int f(){return 0;} }` | `';' が必要です（"N" が見つかりました）` |
| **`using` 宣言 / ディレクティブ** | `using N::f;` | 同上 |
| ~~**`new` / `delete`**~~ | — | **G4 で対応済み**（2026-08-22）。`new Class(args)` / `delete p` / `new POD[n]` / `delete[]`。未対応は scalar POD の `new int`、ctor/dtor 付きクラスの `new C[n]`、`operator new` の置き換え（いずれも明示エラー） |
| **例外** `try` / `catch` / `throw` | `try { throw 1; } catch(int e) {}` | `'try' は宣言されていません` |
| **キャスト演算子** `static_cast` 等 | `static_cast<int>(d)` | `'static_cast' は宣言されていません` |
| `dynamic_cast` | `dynamic_cast<D*>(p)` | `'dynamic_cast' は宣言されていません` |
| **RTTI** `typeid` / `<typeinfo>` | `#include <typeinfo>` | `インクルードファイル 'typeinfo' が見つかりません` |
| **標準ライブラリ** `<iostream>` 等 | `#include <iostream>` | `インクルードファイル 'iostream' が見つかりません` |
| ~~**純粋仮想関数 / 抽象クラス**~~ | — | **G5 で対応済み**（2026-08-22）。抽象クラスのオブジェクト宣言・`new` は明示エラー（ポインタ／参照は可）。未対応: 未 override スロットの実行時スタブ（NULL のまま。抽象判定で到達不能） |
| ~~**仮想デストラクタ**~~ | — | **G6 で対応済み**（2026-08-22）。`delete base_ptr` の動的ディスパッチ + MI の complete-object free（offset-to-top） |
| **仮想継承** | `struct B : virtual public A {}` | `unknown base class` |
| **`friend` 関数宣言** | `friend int peek(P&);` | `unsupported friend declaration`（G2 で明示エラー化。読み捨てると関数宣言自体が消えるため） |
| **`explicit`** | `explicit P(int x) {...}` | `';' が必要です（"explicit" が見つかりました）` |
| **`mutable`** | `mutable int v;` | `';' が必要です（"mutable" が見つかりました）` |
| **クラス内 `enum`** | `struct C{ enum E{A,B}; };` | `identifier が必要です` |
| **デフォルトメンバ初期化子**（C++11） | `struct P{ int v = 5; };` | `',' が必要です（"=" が見つかりました）` |
| **`operator new` のオーバーロード** | `static void* operator new(...)` | `unsupported operator` |
| **暗黙のコピー代入**（メンバ／基底のメンバが `operator=` を持つ場合） | `struct M{ M& operator=(const M&); }; struct H{ M m; H(); }; H x,y; y = x;` | `implicit copy assignment is unsupported for a class whose member declares operator=; declare operator= for this class`。C++98 の暗黙コピー代入はメンバワイズだが tpp の struct 代入は memcpy なので、メンバの `operator=` が静かに飛ばされる（ヒープを持つメンバでは代入先バッファが漏れてコピー元と共有される）。**回避**: そのクラスに `operator=` を書く。POD 代入・`operator=` を書いたクラス自身の代入は不変 |
| **派生 → 基底のスライシング（値）** | `struct D : B {}; D d; B b = d;` / `void take(B); take(d);` / `B f(const D& s){ return s; }` | コピー初期化は `slicing copy-initialization is unsupported; use direct-initialization`、値渡し・`return` は `'const struct D' から 'struct B' に変換できません`。**いずれも fail-closed**（BUG-49）。tpp は struct の引数・戻り値を copy ctor でなく memcpy で運ぶため、値スライスは派生側の vptr を基底オブジェクトへ運び込んでしまう。**回避**: 参照 `const B&` で渡すか、直接初期化 `B b(d);` を使う（どちらも正しく動く） |
| `extern "C++" { ... }` | | 明示エラー |
| `extern "C" { #include ... }` | | 未対応 |

### 3.2 ビルドは通るが**動作が誤っている**（最も注意が必要）

エラーにならないため気付きにくい。**該当する書き方を避けること。**

| # | 機能 | 最小再現 | 期待 | 実際 |
|---|---|---|---|---|
| ~~A~~ | ~~**コピー初期化 `T b = a;` でコピーコンストラクタが呼ばれない**~~ | `P a; P b = a;` | `b.v == 2` | **FEAT-COPY-INIT で対応済み**（2026-08-29）。コピー ctor を解決して呼ぶ |
| B | **委譲コンストラクタの本体が走らない**（C++11） | `P() : P(3) {}` | `p.v == 3` | **`p.v == 0`** |
| C | **`return` 経路の自動デストラクタが呼ばれない** | `int f(){ P p; return p.v; }` | dtor 実行 | **未実行**（ブロック `}` 終了時のみ） |
| D | **ローカル `static` オブジェクトの ctor が走らない** | `static P s;`（関数内） | `s.v == 7` | **`s.v == 0`** |
| ~~A3~~ | ~~**派生 → 基底の変換でコピー ctor が多重適用される**~~（BUG-49） | `void f(const B&); f(d);` / `B x(d);` | コピーなし / 101 | **修正済み**（2026-08-29）。参照束縛は 401 → コピーなし、直接初期化は 501 → 101。**値変換**（値渡し・`return`）は誤コードを避けて明示エラー化（§3.1） |
| ~~A2~~ | ~~**暗黙のコピー代入演算子がメンバワイズにならない**~~ | `operator=` を持つ `M` をメンバに持ち、自身は `operator=` を書いていない `H` で `y = x;` | メンバの `M::operator=` が走る | **明示エラーに変更**（2026-08-29）。memcpy で静かに `M::operator=` を飛ばしていたのを fail-closed 化（§3.1）。`operator=` を書いたクラス自身の代入・POD 代入は従来どおり |
> **A は FEAT-COPY-INIT で解消**（2026-08-29）。残っていた 1 経路が
> コピー初期化 `T b = a;` で、これを直接初期化 `T b(a);` と同じ構築へ接続した。
> ユーザー定義コピー ctor があればオーバーロード解決して呼び、無い場合も
> C++98 の暗黙コピー ctor に相当するメンバワイズ再構築を行う（`new T(obj)` の
> BUG-46 / BUG-47 と同じ手順なので、ヒープバッファを持つメンバが
> エイリアスして二重解放になることもない）。回帰テストは
> `dev/test/a9/copy_init_ctor.cpp` / `copy_init_memberwise.cpp` /
> `copy_init_pod.cpp`。
> これでクラスのコピーは**コピー初期化・直接初期化・`operator=` による代入・
> `new T(obj)`** のいずれも C++98 どおりに動く。引数渡し・`return`・mem-init で
> クラス型が要求される位置は G-CONV の変換 ctor 適用（1 段）が担当する。

> **BUG-30 / BUG-31 は修正済み**（2026-08-22）。BUG-30: メンバ関数を定義より前に
> 呼ぶと（`.h` 宣言＋`.cpp` 定義の通常構成を含む）、単一メンバは実行時クラッシュ、
> オーバーロードは誤解決していた。BUG-31: 静的データメンバのクラス外定義が次の
> 関数定義へクラスを漏らし、暗黙 `this` で引数が 1 つずれていた。
> 回帰テストは `dev/test/a9/govl_*.cpp` / `bug31_static_member_leak.cpp`。

> **BUG-23 は修正済み**（`8841ce4`）。`bump(v + 1)` のように引数式が `this` を
> 参照する呼び出しが実行時クラッシュしていた（`this->bump(this->v + 1)` も同様）。
> `this` を専用スタックスロットへ退避し、引数評価でレジスタが壊れないようにした。
> 回帰テストは `dev/test/a9/bug23_this_in_arg.cpp`。

### 3.3 名前の衝突に関する制限

| # | 制限 | 最小再現 | 診断 |
|---|---|---|---|
| BUG-25 | 同一グローバルスコープで、クラス定義の**後**に同名の変数を宣言できない | `struct tex{...}; int tex = 5;` | `再定義: 'tex'` |
| BUG-26 | グローバル変数を、**どれかのクラスのメソッド名**と同名にできない | `struct O{ void w(){} }; int w = 7;` | `再定義の型が互換していません: 'w'` |

いずれも C++ では合法（MSVC は受理）。原因は、クラス内インライン本体を
グローバル関数へ引き上げる実装と、クラス名の暗黙 typedef 注入にある。

**回避**: 逆順（変数を先に宣言）なら通る。関数内スコープなら通る。

### 3.4 `extern "C"` の制限

tpp の `extern "C"` は**リンケージの変更に加えて、C++ キーワードの字句解析も止める**。
このため `extern "C"` の内側に C++ 固有の構文を書けない。

```cpp
extern "C" {
    class GLUnurbs;   // NG: class が識別子として解析される
}
```

**帰結**: SDK ヘッダがこの形を使っている場合、そのヘッダを個別に
`#if defined(__cplusplus) && !defined(__TINYC__)` で guard する必要がある。
実例として `dev/include/GL/glu.h` を対応済み。

### 3.5 COM / Direct3D は既定で C 形式になる

`dev/include/_mingw.h` は `__TINYC__` のとき `CINTERFACE` / `D3D10_NO_HELPERS` /
`D3D11_NO_HELPERS` を自動定義する。C++ 形式（クラス・テンプレート・`__uuidof`）を
本実装が解析できないためだが、**これは C++ TU から見える API そのものを変える**。

```cpp
p->Release();            // 通常の C++ COM
p->lpVtbl->Release(p);   // tpp ではこちら（CINTERFACE 形式）
```

D3D の C++ ヘルパークラス・演算子も消える。`TCC_NO_FORCE_CINTERFACE` を定義すれば
この強制を解除できるが、テンプレートと `__uuidof` が未実装のうちは解析に失敗する
見込みである（選択をユーザーに残すための出口）。

---

## 4. 精度・意味付けの制限（動くが C++ 標準どおりではない）

| 項目 | 実装 | 標準との差 |
|---|---|---|
| オーバーロード解決 | スコア 2 段（完全一致 10 / 変換 1）。同点はチェーン先頭勝ち | 標準の昇格・変換順位や ambiguous エラーは未実装 |
| アクセス指定子 | パースのみ | `private` / `protected` の**アクセス制御は行われない**（違反してもエラーにならない） |
| 継承 | 単一・多重（非仮想）まで | **菱形継承・仮想継承は未対応** |
| 仮想 + 多重継承 | Phase 2 まで | 深い再 override（孫が祖父母の第 2 基底の仮想を新規 override）はセカンダリ vtable が中間クラスのまま。仮想 dtor の基底経由ディスパッチ未対応。ダウンキャストは調整なし |
| **非 primary 基底のオーバーロード仮想** | **明示エラー** | override の対応付けが名前のみのため解決できない。誤コードを出さずコンパイルエラーにする（`overloaded virtual '...' on a non-primary base is not supported`）|
| クラス名・基底名が長すぎる場合 | **明示エラー** | セカンダリ vtable シンボル名が 256 文字を超えるとエラー停止（旧: 無音で誤コード） |
| メンバ関数ポインタ | 単一継承の embedded base まで | 多重継承 PMF は未対応 |
| 暗黙の ctor / dtor | 派生クラス自身が ctor / dtor を**宣言している場合のみ**、基底とクラス型データメンバへ連鎖（メンバ連鎖は 2026-08-23 対応。mem-init が挙げないメンバを黙示構築し、逆順に破棄。実測: 外側が `ctor`/`dtor` を宣言していればメンバの ctor/dtor が 1 回ずつ走る） | 暗黙 ctor/dtor **そのものの合成**は未対応。ただし silent ではなく **`implicit default construction of non-trivial member is unsupported` で明示エラー**（fail-closed。2026-08-29 実測） |
| 基底の 0 引数 ctor 不在 | **明示エラー** `base class has no default constructor`（旧: 黙って無呼び出し。2026-08-29 実測で fail-closed 化を確認） | 標準どおりエラーになる |
| ctor 有りで 0 引数 ctor 無しの `P f;` | **無警告で確保のみ**（2026-08-29 実測で未解消。既存テスト保護のため据え置き — [実装済み.md](実装済み.md) §8） | 標準ではエラー |
| 非メンバ `operator[]` | 使用可 | **TCC 拡張**（C++98 標準外） |
| グローバル ctor/dtor | EXE のみ | DLL 対象外、`-run` 対象外。`.o` 分割リンク時は同一実行での compile が必要 |

---

## 5. C++ を使うときの実務上の注意

1. **標準ライブラリは使えない**。`<iostream>` / `<string>` / `<vector>` などは無い。
   C 標準ライブラリ（`<stdio.h>` / `<string.h>` / `<math.h>` 等）と Win32 API を使う。
2. **`new` / `delete` は使える**（G4、2026-08-22）。ただし **scalar POD の `new int`**、
   **ctor/dtor を持つクラスの `new C[n]` / `delete[]`**、**`operator new` の置き換え**は
   明示エラーになる。これらが要る場面では `malloc` / `free` を使う。
3. **エラー処理は戻り値で行う**。例外は使えない。
4. **多態は「単一継承 + 仮想関数」を基本にする**。仮想デストラクタと
   `delete base_ptr` の complete-object 解放は G6（2026-08-22）で対応済みだが、
   **仮想継承・菱形継承は未対応**で、多重継承下の深い再 override や
   非 primary 基底のオーバーロード仮想は明示エラー（§4）になる。
5. **クラスの「構築時の」コピーは通常どおり書ける**（FEAT-COPY-INIT、2026-08-29）。
   `P b = a;` / `P b(a);` / `new P(a)` のいずれもコピー ctor（無ければ暗黙の
   メンバワイズコピー）が走る。**代入 `b = a;` は `operator=` を自分で書く**こと。
   `operator=` を持つメンバを含むクラスで暗黙のコピー代入に頼ると、
   メンバワイズにならないため**コンパイルエラー**になる（§3.1。誤コードを
   出さないための措置）。POD の代入は従来どおり動く。
6. **メンバ呼び出しの引数に `this` 由来の式を直接書かない**（§3.2 E）。
   一度ローカル変数に受けてから渡す。
7. **日本語コメントを含む `.cpp` は CP932 で保存するか、各行を ASCII 文字で終える**。
   tcc はソースを CP932 として読むため、UTF-8 の日本語行末（`。` など）が
   改行を食って次行ごとコメント化されることがある。UTF-8 BOM も認識しない。

---

## 6. 参照実装との突き合わせ方

C++ の挙動が疑わしいときは MSVC を参照実装として使う。
「MSVC が通るのに tpp が落ちる」なら tpp 側のバグである。

```powershell
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /EHsc /c <file>.cpp
```

本書 §3 の各項目はこの方法で「合法な C++ である」ことを確認済み。

---

## 7. 既知バグの参照番号

| 番号 | 内容 | 状態 |
|---|---|---|
| BUG-20 | 隠蔽された名前を型と誤判定（引数・内側 typedef・グローバル ctor 引数） | **修正済み** `ad1a4e7` |
| BUG-21 | メンバ関数内でクラススコープがグローバルより優先されない | **修正済み** `a244e65` |
| BUG-22 | 無修飾のメンバ関数呼び出しに `this` が渡らない | **修正済み** `a244e65` |
| BUG-23 | 引数式が `this` を参照する呼び出しが落ちる | **修正済み** `8841ce4`（専用スタックスロットへ退避） |
| BUG-24 | 仮想デストラクタが解析できない | **修正済み**（G6。`delete` の動的ディスパッチ + complete-object free 込み） |
| BUG-25 | `struct X{}; int X;` が誤 redefinition | 未修正（§3.3） |
| BUG-26 | グローバル変数をクラスのメソッド名と同名にできない | 未修正（§3.3） |
| BUG-27 | thunk 名が 256 文字超で raw impl へフォールバックし `this` 未補正 | **修正済み** `7351a0d`（serial 命名＋エラー化） |
| BUG-28 | 非 primary 基底のオーバーロード仮想が同一 thunk へ潰れる | **修正済み** `7351a0d`（明示エラー化） |
| BUG-29 | 非 primary 基底への NULL アップキャストが NULL でなくなる | **修正済み** `7351a0d`（branchless で NULL 保持） |
| BUG-30 | メンバ関数の**前方参照呼び出し**が壊れる（単一なら実行時クラッシュ、オーバーロードなら誤解決） | **修正済み**（ガイド G-OVL。extern 参照の生成 + 宣言側からの候補集合） |
| BUG-31 | 静的データメンバのクラス外定義が、次の関数定義へクラスを漏らし暗黙 `this` を付ける | **修正済み**（`decl()` の宣言子ループ末尾で `cpp_qualified_class` を後始末） |
| BUG-32 | デフォルト引数の再生 3 件（保存列の use-after-free / 呼び出し直後トークンの消失 / 解決がデフォルト引数を無視） | **修正済み**（複製再生・トークン退避・デフォルト考慮の候補判定） |
| BUG-33 | 静的メンバが TU をまたげない（宣言のみで解決不可 / 定義が内部リンケージ） | **修正済み**（extern 参照の生成 + `FuncAttr.func_static_member`） |
| BUG-34 | 祖父母の仮想を孫が override すると別 vtable スロットになり基底実装が呼ばれる | **修正済み**（primary 基底チェーンを辿る `cpp_find_inherited_virtual_slot`） |
| BUG-35 | 自クラスを値返しするメンバを持つクラスで **tcc 自身がスタックオーバーフロー**（診断なしで即死） | **修正済み**（`sym_copy_ref` がプロトタイプ Sym の FuncAttr を `sym_scope` と誤読していた） |
| BUG-36 | ネスト型宣言 `struct Inner {...};` が外側クラスの**匿名メンバ**になりレイアウト肥大 + 同名メンバの重複誤検出 | **修正済み**（C++ では名前付きタグの宣言子なし宣言を型宣言のみとする） |
| BUG-37 | `sym_copy_ref` がファイルスコープのクラスタグを複製し「Iterator を Iterator に変換できません」（同名の別 Sym） | **修正済み**（STRUCT 降下判定を変数の scope から**タグの scope**へ） |
| BUG-38 | メンバ呼び出しの採点で基底の同名関数が派生 override に同点先勝ちし基底実装が呼ばれる（silent miscompile） | **修正済み**（名前隠蔽 — 自クラスレベルに宣言があれば基底へ降下しない） |
| BUG-39 | 別名 typedef 修飾のネスト型宣言 `cu_List::iterator p;` が式に誤送され「static member not found」 | **修正済み**（`cpp_unget_scoped_expr` に typedef 別名→タグ解決を追加） |
| BUG-40 | extern "C" ブロック直後の `#define` 本体が C 字句で貯蔵され C++ キーワードが識別子化（`cu_CATCH_ALL if (false)`） | **修正済み**（終端処理を `lex_c--` → `next()` の順に変更） |
| BUG-41 | 引数リスト内の入れ子メンバ呼び出し `insert(begin(), ...)` が外側の this を消費し全引数が 1 スロットずれて AV | **修正済み**（'(' ハンドラで cpp_member_this/pending を退避・復元） |
| BUG-42 | 局所クラス（関数内定義）のインライン本体が TU 終端再生時にダングリング（タグ/フィールドが local_stack で解放済み） | **修正済み**（クラス定義 Sym を global_stack へ格納。**制限**: タグ名が関数終了後もファイルスコープに残るため、同名の局所クラスを複数関数で定義不可） |
| BUG-43 | クラス外定義の仮想関数が vtable 構築時点で未解決 → 初回呼び出しで NULL 関数ポインタ経由のクラッシュ | **修正済み**（`cpp_lookup_virtual_impl` が既存グローバル未検出時に BUG-30 式の extern 作成へフォールバック。純粋仮想は対象外） |
| BUG-44 | 全パラメータにデフォルトがある 1 引数以上の ctor が `cpp_class_has_default_ctor` で「既定 ctor でない」と誤判定され、`T t;` が ctor 呼び出しごと欠落（メンバ未初期化） | **修正済み**（ゼロ引数で呼べるか判定する `cpp_ctor_viable_with_zero_args` へ置換。副次的に `cpp_inherit_decl_defaults` のコンストラクタ探索がマングルトークン↔クラス名トークンの対応漏れで別オーバーロードを拾うバグも修正） |
| BUG-45 | 同クラスの static メンバ関数への非修飾呼び出しが、定義より前の位置からだと C の「暗黙関数宣言」（無型 K&R）経路に落ちて呼び出し規約が不一致になり不正アドレスへジャンプ | **修正済み**（`cpp_lookup_static_member`（BUG-33 の extern 生成機構）で解決してから通常の解決済みシンボル経路へフォールスルー） |
| BUG-46 | `new T(obj)` の暗黙コピー構築がメンバワイズでなくフラットなバイトコピーで、ヒープバッファを持つメンバが共有され二重解放（→ CRT ヒープ破壊 → 無関係な alloc/free でハング） | **修正済み**（`cpp_reconstruct_copied_class_members` を新設し、ctor を持つクラス型メンバをコピー用 ctor で再構築。2026-08-24） |
| BUG-47 | BUG-46 の再構築呼び出しが引数 1 個固定で、既定引数付きパラメータを持つコピー用 ctor（`SimpleString(const SimpleString&, pos=0, n=npos)`）にゴミ値が渡る | **修正済み**（`cpp_apply_default_args` を同経路にも適用。2026-08-24） |
| BUG-48 | 局所クラスを同一関数内で複数定義すると、最初の 1 つのスコープ終端デストラクタ呼び出しが静かに欠落 | **修正済み**（`cpp_class_sym_push` の global_stack 昇格でメンバ型の `ref` チェーンも `sym_copy_ref` で同じ寿命へコピー。2026-08-25） |
| BUG-49 | 派生 → 基底の変換で、基底のコピー ctor が `cpp_conv_depth` の上限まで再帰適用される。**`const B&` への束縛でも一時が作られ（copy ctor 4 回）、`B&` 経由の書き込みが元オブジェクトへ届かず失われていた** | **修正済み**（2026-08-29）。`cpp_try_class_conversion()` の早期 return を `ref == class_sym` から `cpp_base_subobject_offset() >= 0` へ変更し、派生→基底を参照バインド／通常のコピー経路へ譲る。参照束縛は copy ctor 0 回・書き込みが正しく反映、直接初期化 `B x(d);` は 501 → 101。**値スライスのみ fail-closed**（§3.1） |

過去のバグ事例と原因分析は [問題と原因.md](問題と原因.md)、
残作業と着手順は [amateras対応作業履歴.md](amateras対応作業履歴.md) §5〜6 を参照。
