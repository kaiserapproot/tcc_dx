# tpp の Linux（WSL）ビルド

tpp を Linux x86_64（ELF、SysV ABI）向けに gcc でビルドし、Linux 上で C / C++ をコンパイル・実行する。
Windows ビルド（`tcc.vcxproj` / `build.bat`）とは独立していて、Windows 側の成果物には影響しない。

## 必要なパッケージ（Ubuntu 24.04）

```sh
sudo apt install build-essential libc6-dev       # 必須
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
```

ヘッダとライブラリは Linux の標準パス（`/usr/include`、`/usr/lib/x86_64-linux-gnu`）から探す。
tpp 自身のヘッダ（`tccdefs.h` など）と `libtcc1.a` は、ビルド時に絶対パスで埋め込んだ
`linux/out` から探す（`CONFIG_TCCDIR`）。`linux/out` を別の場所へ移すときは `-B<dir>` を付けるか、作り直す。

## テスト（`linux/run_tests.sh`）

`dev/test/run_all.bat` の中核フェーズを移植したもの。テストのソースは Windows 版と同じものを使う。

| フェーズ | 内容 |
|---|---|
| 0 | `linux/tests/*.c` / `*.cpp`（hello、可変長引数、double 8 引数 ABI、C++ 基本機能）を `-o` と `-run` の両方で実行 |
| 1 | `dev/test` の各ソースを `-c` でコンパイル（`run_all.bat` の Phase 1 と同じ対象と既知の未対応リスト） |
| 2 | `a9/*.cpp` などをビルドして実行し、終了コード 0 を確認 |
| 2b | `-run a9/local_static_dtor.cpp` |
| 3 | `a9/negative`（コンパイルエラーになることと、そのエラーメッセージを確認） |
| 4 | `linux/tests/gl/x11_glx.cpp`: C++ のクラスから X11 ウィンドウと GLX コンテキストを作り、描画したピクセルを読み戻して確認する。ディスプレイがない環境ではスキップ |

移植していないもの: `a9/manual/*.bat` などの Windows 専用ゲート（PE の TLS / 起動処理、SDK / DWARF / asm / SIMD）、
`windows.h` を include するテスト（`WINONLY` で除外）、CUnit / CPPUnit。

クラッシュの扱い: Linux には Windows 版の VEH クラッシュ網に相当するものがない。
シグナルによる終了（終了コード 128 以上）か、stderr の `internal error` をクラッシュとみなして 1 回だけ報告し、再実行はしない。

## 仕組み

- `TCC_TARGET_X86_64` を明示して、`config.h` が PE を選ばないようにしている（`config.h` は Windows と共用のため変更しない）。
- CP932 で保存されているソース（`tccgen.c`、`tccpp.c`、`tccasm.c`、`libtcc.c`）は、`iconv` で UTF-8 に変換したコピー（`linux/out/src/`）からコンパイルする。
  gcc は 2 バイト目が 0x5C の文字をバックスラッシュのエスケープと解釈してしまい、また `tcc.h` が UTF-8 の `x86_64-gen.c` を取り込むので、ファイル単位の `-finput-charset` も使えないため。元のファイルは書き換えない。
  エラーメッセージは UTF-8 で出力される。
- `include/stdarg.h` は古い形式で `tccdefs.h` と食い違うため、Linux 用には `linux/include/stdarg.h` で上書きする。
- `libtcc1.a` は、ビルドした tcc 自身で `lib/libtcc1.c`、`lib/va_list.c`、`lib/alloca86_64*.S`、`linux/lib/dsohandle.c` をコンパイルして作る。
  `dsohandle.c` は、`atexit` が参照する `__dso_handle` を提供する（gcc の場合は crtbegin.o が提供するが、tcc は crtbegin.o をリンクしない）。

## Linux での C++ の違い

| 項目 | Linux | Windows |
|---|---|---|
| グローバルの ctor / dtor | `.init_array` / `.fini_array` を glibc の crt が実行する。デストラクタの登録は `atexit` で行う | tpp が注入する起動コード（`_tcc_cpp_start`）が実行する |
| `-run` の起動処理 | `tcc_add_runmain`（Windows と共通） | 同左 |
| 仮想多重継承の this 調整サンク | `sub rdi, imm32`（SysV の第 1 引数）。**構造体を返す仮想関数は明示エラー**（hidden pointer が第 1 引数になり、`this` が第 2 引数へずれるため） | `sub rcx, imm32` |
| `thread_local`（C++） | **明示エラー**（`thread_local requires the x86-64 PE target`）。Windows の TLS ランタイムとサンクが Win64 専用のため | 対応（N6） |
| `long` | 64 ビット（LP64）。名前修飾では `long` と `long long` を区別する | 32 ビット（LLP64） |
| 名前修飾 | tpp 独自形式（Windows と同じ）。g++ が作った C++ オブジェクトとはリンクできないので、外部ライブラリとは `extern "C"` で接続する | 同左 |

## 既知の問題（ターゲット共通）

- **派生クラスが基底と同じ名前のメンバ関数を、`virtual` を付けずに定義すると、派生側の関数が呼ばれない**
  （デストラクタは除く）。コンパイルは通り、基底の実装が呼ばれる。Windows 版の `dev\tcc.exe` でも同じ結果になる。
  次のコードで g++ -std=c++98 と比べた（`v` は 21、派生の `get()` は `v * 2` を返す）。

  | 基底の `get()` | 派生の `get()` | 呼び出し | g++ | tpp |
  |---|---|---|---|---|
  | virtual | `virtual` なし | `d.get()`（直接） | 42 | **21** |
  | virtual | `virtual` なし | `base_ptr->get()` | 42 | **21** |
  | 非 virtual | 非 virtual | `d.get()`（直接） | 42 | **21** |
  | 非 virtual | 非 virtual | `base_ptr->get()` | 21 | 21 |

  C++ では、派生側の同名関数が基底の関数を隠すので、直接呼び出しでは派生側が呼ばれる。
  基底の関数が virtual なら、派生側の同名・同引数の関数はキーワードがなくても override になる（C++98 10.3/2）。
  派生側に `virtual` を付けた場合だけ、正しく動く。
- 基底へのポインタ変換（`Base *p = &derived;`）で「互換性のないポインタ型からの代入です」という警告が出る（動作は正しい）。
