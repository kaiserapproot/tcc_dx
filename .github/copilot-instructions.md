# プロジェクトガイドライン

> **詳細は [AGENTS.md](../AGENTS.md) / [CLAUDE.md](../CLAUDE.md) を参照。**
> このファイルは Windows 操作ルールと最重要のプロジェクト固有ルールのみ。

## Windows ターミナル操作ガイドライン

- **実行ファイル**: カレントディレクトリのバイナリ実行には必ず `./` または `.\` を付けること。
- **バイナリ解析**: 自作スクリプトを避け `certutil -encodehex <file> out.hex` を使用すること。
- **クォート**: `powershell -Command` での複雑なエスケープを避けること。
- **コマンド**: `ls` → `dir`, `grep` → `findstr`, `rm -rf` → `Remove-Item -Recurse`, `head` → `gc -Head` を徹底すること。
- **Python を使うな。環境にインストールされていない**

## プロジェクト固有ルール（必須）

このリポジトリは TCC (Tiny C Compiler) を MSVC でビルドして C++ 機能を段階追加するプロジェクト。
**過去に「ビルドできない」「ソースが壊れる」「テストが動かない」が繰り返し発生**しているため以下を厳守する。

### 規律

- **1 ガイド = 1 feature branch = 1 コミット**。master 直接作業禁止。
- **コミット前に必ず `build.bat` が緑**。壊れた状態でコミットしない。
- **`git checkout <file>` で破損ファイルを巻き戻すのは禁止**。branch ごと破棄してタグ `clean-baseline-2026-05-24` から再 branch する。
- `--no-verify` で pre-commit をバイパスしない。

### `tccgen.c` の改変

- 関数単位の最小差分のみ。一括書き換え禁止。
- 新規変数は **ブロック先頭にまとめて宣言**（MSVC C90 厳格性、`error C2065` の原因）。
- `struct_decl()` 内で `gen_function()` を呼ばない。メンバ関数本体は `skip_or_save_block(NULL)` で読み捨て。

### ビルド構成

- `ONE_SOURCE=0` を維持。全 `.c`（`tccpp.c`, `tccgen.c`, `tccdbg.c`, `i386-link.c` 等）を個別 `<ClCompile>` 登録。
- `tcctools.c` は vcxproj に追加しない（`tcc.c:29` で `#include` 済み → LNK2005）。
- `x86_64-gen.c` の `gfunc_call` 内 `o(0xc0 + arg*8 + REG_VALUE(d));` の **`arg*8` を消さない**（double 6 引数 ABI 修正、[履歴.md](../履歴.md) 2026-05-23）。
- `dev/test/repro_double6.c` を `build.bat` の必須回帰テストに含める。
- `tccgen.c` を変更したら Release|x64 と Debug|Win32 の両方をビルドする。

### エンコーディング

- 新規ファイル（`dev/test/` 等）: UTF-8 with BOM 可。
- 既存 TCC ソース（`tcc.c`, `tccgen.c`, `tccpp.c`, `tcc.h`, `*-gen.c` 等）: **ASCII 維持**。日本語コメントを書かない。
- 一括 BOM 化禁止。

### C++ 実装

- C++ 機能は `s1->cpp == 1` のときのみ有効。**`.c` の従来動作を絶対に壊さない**。
- キーワード降格は **`TokenSym.alt_ident_tok` + `tok_alloc_demote()` 方式**（[IMPLEMENTATION_GUIDE.md](../IMPLEMENTATION_GUIDE.md) A-2）。
  - `tok_alloc()` / `tok_alloc_new()` を降格に使わない（前者は hash ヒットで TOK_CLASS が返る、後者は hash 汚染で `.cpp` が壊れる）。
- Stage 1 の `extern "C"` は **ブロック構文のみ**。単一宣言と `extern "C++"` は Stage 2。
- 修飾名 `::` / `Class::method` は Stage 2 以降。

### テスト / ビルドフロー

- `build.bat` は msbuild → dev コピー → smoke → `repro_double6` → CUnit → `run_all.bat` を errorlevel 0 で通す。
- `dev/test/run_all.bat` は先頭で必ず `pushd "%~dp0"`。
- `dev/test/aN/` 内のソースは **そのガイド時点の機能だけ**で動くものに限定。

### 失敗時に最初に確認

| 症状 | 確認 |
|------|------|
| LNK2005 重複定義 | `ONE_SOURCE=0` と全 `.c` 個別コンパイル |
| LNK2019 `tccgen_compile` 等 | `tccgen.c` 破損 → branch 破棄、タグから再開 |
| C4819 / 文字化け | 既存 `.c` に BOM 付加していないか |
| C2065 未宣言 | 変数宣言をブロック先頭に |
| `.cpp` が bin 扱い | A-1 `guess_filetype()` 未適用 |
| `.c` で `class` がエラー | A-2 降格未適用 |

詳細な切り分け・事前 grep・参照ドキュメントは [AGENTS.md](../AGENTS.md) を参照。
