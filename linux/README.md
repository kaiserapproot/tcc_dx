# tpp の Linux（WSL）ビルド

tpp を Linux x86_64（ELF、SysV ABI）向けに gcc でビルドし、Linux 上で C / C++ をコンパイル・実行する。
Windows ビルド（`tcc.vcxproj` / `build.bat`）とは独立していて、Windows 側の成果物と動作は変えない。

## 必要なパッケージ（Ubuntu 24.04）

```sh
sudo apt install build-essential libc6-dev       # 必須（Phase 7 の ABI 突き合わせにも gcc を使う）
sudo apt install libx11-dev libgl-dev            # X11 / OpenGL のテスト（Phase 4）を回す場合
```

## ビルドとテスト

```sh
make -C linux          # linux/out/tcc, linux/out/include, linux/out/libtcc1.a
make -C linux test     # linux/run_tests.sh
make -C linux clean
```

`linux/out/` はビルドのたびに作り直される生成物で、git の管理対象外。

## 使い方

```sh
linux/out/tcc foo.c -o foo
linux/out/tcc foo.cpp -o foo                 # .cpp は C++ モード（Windows 版と同じ判定）
linux/out/tcc -run foo.cpp                   # メモリ上で実行
linux/out/tcc app.cpp -o app -lX11 -lGL      # X11 + OpenGL（GLX）
linux/out/tcc -shared -fPIC lib.c -o lib.so  # 共有ライブラリ（C++ のデストラクタ実行時処理は不可。後述）
```

ヘッダとライブラリは Linux の標準パス（`/usr/include`、`/usr/lib/x86_64-linux-gnu`）から探す。
tpp 自身のヘッダ（`tccdefs.h` など）と `libtcc1.a` は、ビルド時に絶対パスで埋め込んだ
`linux/out` から探す（`CONFIG_TCCDIR`）。`linux/out` を別の場所へ移すときは `-B<dir>` を付けるか、作り直す。

tpp の `.o` を gcc / GNU ld でリンクするときは、`linux/out/libtcc1.a` も渡す（`__va_arg` などが入っている）。

## テスト（`linux/run_tests.sh`）

Phase 0〜3 は `dev/test/run_all.bat` の中核フェーズを移植したもので、テストのソースは Windows 版と同じものを使う。
Phase 4〜7 は Linux（ELF）固有の確認。

| フェーズ | 内容 |
|---|---|
| 0 | `linux/tests/*.c` / `*.cpp`（hello、可変長引数、double 8 引数、C++ 基本機能、7 個以上の引数を取るグローバル ctor、終了順序）を `-o` と `-run` の両方で実行 |
| 1 | `dev/test` の各ソースを `-c` でコンパイル（`run_all.bat` の Phase 1 と同じ対象と既知の未対応リスト） |
| 2 | `a9/*.cpp` などをビルドして実行し、終了コード 0 を確認（仮想多重継承の `vmi_*.cpp` を含む） |
| 2b | `-run a9/local_static_dtor.cpp` |
| 3 | `a9/negative`: コンパイルエラーになることと、`.expected` のエラーメッセージを確認する。`.expected` がないテストは終了コードだけで判定し、その名前を `[exit-code only, no .expected]` として表示する。`.expected` が空なら失敗 |
| 4 | `linux/tests/gl/x11_glx.cpp`: C++ のクラスから X11 ウィンドウと GLX コンテキストを作り、描画したピクセルを読み戻して確認する。ディスプレイがない環境ではスキップ |
| 5 | ELF のリンク: 複数ファイルの C / C++（グローバル ctor / dtor と多重定義を別 TU に置く）、`-ar` で作った `.a`、`-shared` で直接作った `.so` と `-c` を経た `.so`、`dlopen` → `dlsym` → `dlclose` を 2 回（読み込みごとに `.init_array` / `.fini_array` が 1 回ずつ実行されること）、リンク時に `-l` で指定した `.so`、C++ の `.so`（ctor のみ）、`atexit` を呼ぶ C の `.so`（直接作る場合と `-c` を経る場合のそれぞれで、`dlopen` → `dlclose` を 2 回行い、登録した関数が毎回 `dlclose` のときに実行され、終了時に落ちないこと）。C++ のデストラクタ実行時処理を含む `.so` が両方の経路で拒否されること |
| 6 | 終了順序: `main` からの return と `exit()` のそれぞれで、`-o` と `-run` の両方が glibc と同じ順序（`atexit` で登録した関数 → C++ のグローバル dtor → `.fini_array`）になること |
| 7 | SysV ABI の突き合わせ: gcc で作ったオブジェクトと tpp で作ったオブジェクトを、呼ぶ側・呼ばれる側を入れ替えて相互に呼ぶ（整数 7 個以上・double 9 個以上の引数、可変長引数、`va_copy`、各種構造体の値渡しと戻り値（float / double の配列メンバ、packed 構造体を含む）、スタックに溢れる構造体、コールバック）。gcc / GNU ld でのリンクも、tpp のオブジェクトを外部のリンカが扱えることの確認になる。あわせて、SSE と INTEGER が混在する構造体の値渡し・戻り値の 4 通りが拒否され、ポインタ渡しと 8 バイト以下の混在は通ること（`abi_mixed_reject.c`） |

作業ディレクトリは実行ごとに `linux/out/` の下へ `mktemp -d` で作り、終了時に削除する（並列実行しても衝突しない。`TPP_TEST_TMPDIR` で場所を変えられる）。
生成したプログラムの終了コードは、0 以外をすべて数える: 1〜123 は失敗、124 はタイムアウト（クラッシュ扱い）、125〜127 は実行不能（失敗）、128 以上はシグナル（クラッシュ）。

移植していないもの: `a9/manual/*.bat` などの Windows 専用ゲート（PE の TLS / 起動処理、SDK / DWARF / asm / SIMD）、
`windows.h` を include するテスト（`WINONLY` で除外）、CUnit / CPPUnit。

クラッシュの扱い: Linux には Windows 版の VEH クラッシュ網に相当するものがない。
コンパイラと生成したプログラムのどちらについても、シグナルによる終了（終了コード 128 以上）とタイムアウト、
およびコンパイラの stderr の `internal error` をクラッシュとして数え、1 回だけ報告して再実行はしない。

## 仕組み

- `TCC_TARGET_X86_64` を明示して、`config.h` が PE を選ばないようにしている（`config.h` は Windows と共用のため変更しない）。
- CP932 で保存されているソース（`tccgen.c`、`tccpp.c`、`tccasm.c`、`libtcc.c`）は、`iconv` で UTF-8 に変換したコピー（`linux/out/src/`）からコンパイルする。
  gcc は 2 バイト目が 0x5C の文字をバックスラッシュのエスケープと解釈してしまい、また `tcc.h` が UTF-8 の `x86_64-gen.c` を取り込むので、ファイル単位の `-finput-charset` も使えないため。元のファイルは書き換えない。
  エラーメッセージは UTF-8 で出力される。
- `linux/include/sys/cdefs.h`: glibc の `<sys/cdefs.h>` は `__GNUC__` が未定義だと `#define __attribute__(xyz)` を行い、
  以降の `__attribute__` をすべて消してしまう（`__attribute__((constructor/destructor))` が `.init_array` / `.fini_array` に入らなかった）。
  本物を `#include_next` で読み込んだ直後に `__attribute__` を `#undef` するラッパーを、`/usr/include` より先に置く。
- `include/stdarg.h` の Windows 以外の x86_64 部分は、`tccdefs.h` の `__builtin_va_*` を使う（Windows は `dev/include` を使うので影響しない）。
  `lib/va_list.c` は `__va_arg` に加えて、古い `stdarg.h` で作ったオブジェクト向けに `__va_start` も提供する。
- `libtcc1.a` は、ビルドした tcc 自身で `lib/libtcc1.c`、`lib/va_list.c`、`lib/alloca86_64*.S`、`linux/lib/dsohandle.c` をコンパイルして作る。
  `dsohandle.c` は、gcc では crtbegin.o / crtbeginS.o が担う 2 つの役割を持つ（tcc はこれらをリンクしない）。
  - `atexit` が参照する `__dso_handle` を提供する（`.so` ごとに別の値になる hidden シンボル）。
  - `.fini_array` から `__cxa_finalize(__dso_handle)` を呼ぶ。これで、`.so` の中で `atexit` に登録した関数が `dlclose` のときに実行されて登録から外れる。
    これがないと、登録が解放後のコードを指したまま残り、プロセスの終了時に SIGSEGV で落ちる。
    実行ファイルでは、`exit()` が `.fini_array` より先に登録済みの関数をすべて実行して外すので、この呼び出しは何もしない。
- `.eh_frame` の FDE の開始アドレスは、RELA の `r_addend` に置く（tccdbg.c）。以前はセクションのデータ側に置いていて、tpp 自身のリンカでは正しく
  リンクできたが、GNU ld は `r_addend` しか読まないので、すべての FDE が `.text+0` を指し、`overlapping FDEs` でリンクに失敗していた。
  tpp 自身のリンカはデータと `r_addend` を合算するので、tpp でリンクした結果は変わらない。

## Linux での C++ の違い

| 項目 | Linux | Windows |
|---|---|---|
| グローバルの ctor / dtor | `.init_array` を glibc の crt が実行する。デストラクタは ctor の実行時に `atexit` で登録する | tpp が注入する起動コード（`_tcc_cpp_start`）が実行する |
| 終了順序 | glibc と同じ（`atexit` で登録した関数、C++ のグローバル dtor → `.fini_array`）。`-run` も同じ順序にしている | `.fini_array` → `atexit` で登録した関数（FEAT-4G のまま変更しない） |
| `-run` の起動処理 | `tcc_add_runmain`（Windows と共通。終了順序だけ上記のとおり異なる） | 同左 |
| 共有ライブラリ（`.so` / DLL） | C++ のデストラクタ実行時処理を含むものは**明示エラー**（`-shared` で直接作る場合も、`-c` を経てリンクする場合も）。PE と同じ方針で、`-shared` でコンパイルした時点の拒否は `tccgen.c`（ターゲット共通）にある。`dsohandle.c` で `dlclose` 時の `__cxa_finalize` は行うようになったので、解除は可能だが、`tccgen.c` の変更と専用のテストが要るので別のガイドにする。ctor だけの C++ や C の `.so` は使える。C の `.so` の中で呼んだ `atexit` は、`dsohandle.c` の `__cxa_finalize` によって `dlclose` のときに実行される | DLL は同じく明示エラー |
| 仮想多重継承の this 調整サンク | `sub rdi, imm32`（SysV の第 1 引数）。**構造体を返す仮想関数は明示エラー**（hidden pointer が第 1 引数になり、`this` が第 2 引数へずれるため。レジスタで返る小さい構造体も含めて安全側で拒否している） | `sub rcx, imm32` |
| `thread_local`（C++） | **明示エラー**（`thread_local requires the x86-64 PE target`）。Windows の TLS ランタイムとサンクが Win64 専用のため | 対応（N6） |
| `long` | 64 ビット（LP64）。名前修飾では `long` と `long long` を区別する | 32 ビット（LLP64） |
| 名前修飾 | tpp 独自形式（Windows と同じ）。g++ が作った C++ オブジェクトとはリンクできないので、外部ライブラリとは `extern "C"` で接続する | 同左 |

## 既知の問題

### Linux（SysV）固有

- **8 バイトごとに浮動小数点と整数が混在する 16 バイト以下の構造体は、値渡しも戻り値も明示エラーになる**
  （`passing or returning struct '...' by value is unsupported on this target: its eightbytes mix SSE and INTEGER classes`）。
  例: `struct { double d; long l; }`、`struct { int id; float x, y, z; }`。
  SysV ABI では 8 バイトごとに分類して `xmm0` と `rdi` のように渡すが、tpp（tinycc から引き継いだ処理）は構造体全体を
  1 つの分類にまとめて渡すため、gcc で作ったコードとの間で値が黙って壊れていた。ポインタで渡すこと。
  直すには `x86_64-gen.c` の引数・戻り値のコード生成と `tccgen.c` の戻り値処理を、8 バイト単位の分類に対応させる必要がある（別のガイド）。
  拒否されないもの（gcc と互換）: 8 バイト以下の構造体（`struct { float f; int i; }` は INTEGER 1 つで gcc と同じ）、
  float / double だけの構造体（配列メンバを含む。`struct { float v[4]; }` は SSE 2 つ）、整数だけの構造体、16 バイトを超える構造体（メモリ渡し）。
  配列メンバは、以前は INTEGER に分類していて `struct { float v[2]; }` なども gcc と互換でなかったが、要素の型で分類するように直した。
- アラインメントが崩れたメンバを含む構造体（`struct __attribute__((packed)) { char c; double d; }` など）は、
  psABI どおり MEMORY に分類する（値渡しはスタック、戻り値は隠しポインタ）。以前は整数レジスタで渡していて、gcc と互換でなかった。
  packed でもメンバのアラインメントが崩れていない構造体（`struct __attribute__((packed)) { int a, b; }`）は、従来どおりレジスタで渡す。

### ターゲット共通（Windows 版の `dev\tcc.exe` でも同じ結果）

- **派生クラスが基底と同じ名前のメンバ関数を定義すると、基底側の関数が呼ばれる。** コンパイルは通る。
  正しく動くのは、基底と派生の両方に `virtual` が付いている場合だけ。g++ -std=c++98 との比較
  （`v` は 21、派生の `get()` は `v * 2`、さらに派生した `E` の `get()` は `v * 3` を返す）:

  | 基底の `get()` | 派生の `get()` | 呼び出し | g++ | tpp |
  |---|---|---|---|---|
  | 非 virtual | 非 virtual | `d.get()`（直接） | 42 | **21** |
  | 非 virtual | 非 virtual | `base_ptr->get()` | 21 | 21 |
  | 非 virtual | virtual | `d.get()`（直接） | 42 | **21** |
  | 非 virtual | virtual | `E` を派生型のポインタ経由で呼ぶ | 63 | **21** |
  | virtual | `virtual` なし | `d.get()` / `base_ptr->get()` | 42 / 42 | **21 / 21** |
  | virtual | virtual | `d.get()` / `base_ptr->get()` | 42 / 42 | 42 / 42 |

- **コンストラクタを持たず、デストラクタだけを持つクラスのグローバルオブジェクトは、デストラクタが呼ばれない。**
  原因は未調査。tpp はグローバルの dtor を、ctor を呼ぶ初期化処理（`.init_array` のサンク）の中で登録しているので、
  ctor がないとその処理が作られないのではないかと推測している。
- 基底へのポインタ変換（`Base *p = &derived;`）で「互換性のないポインタ型からの代入です」という警告が出る（動作は正しい）。
