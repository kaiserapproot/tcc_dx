# tcc-doc 日本語・暫定改訂版 — 実装者検証指示

## 目的

`/tcc-doc-ja-revised-draft.html` は **正本ではなく検証用の暫定改訂案** です。

元の `tcc-doc.html`（TCC 0.9.28rc）を日本語化し、現在の tpp / TCC DX の
C++・Windows・Amateras 向け拡張を統合しています。

この作業では、**文書に実装を合わせないでください。**
実際のソース、コンパイル結果、リンク結果、実行結果、既存回帰テストを authority とし、
文書が間違っていれば文書側を修正します。

---

## 0. 検証対象を固定する

最初に以下を記録してください。

```text
VERIFY_BRANCH=
VERIFY_COMMIT=
TCC_VERSION=
TARGET=
OS=
BUILD_CONFIG=
```

`git status --short` も記録し、dirty worktree の場合は差分が検証へ影響しないことを確認してください。

**STOP:** 検証対象 SHA が固定できない場合。

---

## 1. 元 `tcc-doc.html` の C/TCC 基本仕様

以下について fork で意味が変わっていないか確認してください。

- command line options
- C / C99
- GNU C extensions
- TinyCC extensions
- assembler
- linker
- `-b` bounds checking
- libtcc
- developer guide の主要内部構造

単に元文書を読み直すのではなく、fork の source diff と option parser を確認してください。

特に次は古い可能性があるため要実測です。

- supported target の説明
- x86 assembler の MMX/SSE 記述
- PE-i386 という章名と現在の Windows x86_64 実装
- PIC / shared library 関連
- bounds checking の target 一覧

**STOP:** 元文書と現在実装で仕様差が見つかった場合。
差分を記録してから先へ進み、元文書側を無条件に正しいとは扱わないでください。

---

## 2. C++ モード入口

source を確認:

- `libtcc.c`
  - `is_cpp_source()`
  - `guess_filetype()`
  - `TCC_OPTION_x`
  - compile 単位ごとの `s1->cpp`
- `tccpp.c`
  - `__cplusplus`
  - C++ keyword / C identifier の切替
- `tcc.h`
  - `TCC_CXX_VERSION`

最低限、以下を実測してください。

```text
foo.c
foo.cpp
foo.cxx
foo.cc
foo.hpp
-x c
-x c++
-x c++-header
```

確認事項:

- `.cpp/.cxx/.cc/.hpp` が本当に C++ 扱いになるか
- `.c` が C のままか
- `.h` が include 元の mode に従うか
- `-x c++` / `-x c++-header` が現在も有効か
- C++ で `__cplusplus == 199711L` か
- C で C++ keyword が識別子として使えるか

---

## 3. C++ ABI / name mangling

文書に記載された以下を source と symbol output の両方で確認してください。

```text
__tcc_<function>_<args>
__tcc_<class>__<method>_<args>
__cpp_ctor_<class>
__cpp_dtor_<class>
__cpp_vtbl_<class>
__cpp_vtbl2_<derived>_<base>
```

確認:

- free function
- overloaded free function
- member function
- const member
- constructor
- destructor
- virtual function / vtable
- `extern "C"`
- `main` / `wmain`

また、MSVC/GCC C++ ABI と直接互換ではない、という文書記述の根拠を確認してください。

**STOP:** 文書の mangling 形式と実シンボルが一致しない場合。

---

## 4. クラス・オブジェクト寿命

既存 gate を優先して再利用し、必要なら最小再現を追加してください。

確認対象:

- class / struct
- member function / `this`
- references
- function overload
- default arguments
- const member
- static data member
- constructor
- implicit default constructor
- copy constructor
- copy initialization
- conversion constructor
- destructor / RAII
- temporary lifetime
- return path
- goto
- break / continue
- local object
- global object
- function-local static object

各ケースは **compile success だけでなく値と ctor/dtor 回数**を確認してください。

---

## 5. class array

確認対象:

- local automatic class array
- multidimensional local class array
- function-local static class array
- member class array
- default argument を使う ctor の array
- default ctor が無い class
- destructor を持つ class array
- `new T[n]` / `delete[]`

「未対応」は compile fail するだけでなく、
**silent acceptance / silent miscompile が無いこと**を確認してください。

---

## 6. `thread_local`

source:

- `tcctok.h`
- `tccgen.c`
- `tcc.h`
- `tccelf.c`
- `tccpe.c`
- `tccrun.c`

実行形態ごとに分けてください。

| mode | 確認 |
|---|---|
| normal EXE | trivial TLS / class TLS / thread exit dtor |
| `-run` / `tcc_run()` | thread wrapper と cleanup |
| libtcc direct relocate | construction / cleanup responsibility |
| DLL | fail-closed / unsupported contract |

確認:

- thread ごとに値が分離される
- lazy ctor
- dtor reverse order
- main thread
- worker thread
- `CreateThread`
- `_beginthreadex`
- `ExitThread`
- `_endthreadex`
- unsupported TLS shape が fail-closed

**STOP:** thread 終了時の破棄漏れ、二重破棄、別 thread の値共有。

---

## 7. Windows SDK / COM / Direct3D

確認 source:

- `dev/include/_mingw.h`
- `_mingw_secapi.h`
- `corecrt.h`
- `stdlib.h`
- `math.h`
- `swprintf.inl`
- COM headers
- `guiddef.h`
- `winbase.h`
- `winuser.h`
- `winnt.h`
- `psdk_inc/intrin-impl.h`
- D3D10 / D3D11 headers

確認:

```text
CINTERFACE
D3D10_NO_HELPERS
D3D11_NO_HELPERS
TCC_NO_FORCE_CINTERFACE
```

特に、C++ TU から見える COM API が

```cpp
p->Release();
```

ではなく既定で

```cpp
p->lpVtbl->Release(p);
```

側になる、という契約が現在も正しいか確認してください。

---

## 8. Amateras consumer qualification

可能な限り実際の Amateras source をそのまま使って確認してください。

最低限:

- `cross.h` C
- `cross.h` C++
- Windows window path
- `win_txt`
- `window_t`
- `render_opengl : public window_t`
- `vec_quat.h`
- MMD + OpenGL
- town local `vec2[4]`
- NIF function-local static class array
- `extern "C"` boundary
- C/C++ mixed TU

build だけでなく、既存 consumer gate があるものは link/run まで実行してください。

---

## 9. 現在記録されている既知 gap

次の2件が現在も再現するか確認してください。

### A. 複数 C++ TU + `windows.h`

`guiddef.h` の `DEFINE_GUID` 系が `extern "C" const GUID name;` を
definition として扱い、複数 TU で GUID symbol が重複する件。

### B. 先行 non-inline declaration + 後続 inline definition

未使用でも inline function が emit され、
body 内の intrinsic / external symbol が未解決になり得る件。

この2件は修正要求ではなく、まず **CURRENT / FIXED / CHANGED** を判定してください。

---

## 10. 文書検証

最終的に `tcc-doc-ja-revised-draft.html` の各記述を

```text
VERIFIED_SOURCE
VERIFIED_COMPILE
VERIFIED_LINK
VERIFIED_RUNTIME
VERIFIED_NEGATIVE
DOC_FIX_REQUIRED
```

のどれで確認したか記録してください。

### 完了条件

```text
DOC_C_BASIC_DIFF_UNEXPLAINED=0
DOC_CPP_SOURCE_MISMATCH=0
DOC_CPP_RUNTIME_MISMATCH=0
DOC_UNSUPPORTED_SILENT_ACCEPTANCE=0
DOC_KNOWN_GAPS_CLASSIFIED=YES
DOC_AMATERAS_CONSUMER_GATE=PASS
```

### 最重要ルール

文書と実装が食い違った場合:

1. その場で実装を変えない。
2. 最小再現を作る。
3. MSVC / upstream TCC /既存 gate のどれを参照 authority にするか明示する。
4. 「実装バグ」「文書バグ」「仕様差」「意図した制限」のどれかを判定する。
5. 判定後に、必要なら別コミットで修正する。
