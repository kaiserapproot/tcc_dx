# プロジェクト指針（AI エージェント共通）

> このファイルは AGENTS.md と CLAUDE.md で同一内容。`.github/copilot-instructions.md` は Windows ルールのみ簡略版。
> 詳細プランは [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)、過去事例は [履歴.md](履歴.md) / [問題と原因.md](問題と原因.md) を参照。

---

## プロジェクト概要

TCC (Tiny C Compiler) を **MSVC / Windows x64** でビルドし、段階的に **C++98 サブセット**を実装するプロジェクト。

| ディレクトリ | 役割 |
|--------------|------|
| `tcc*.c`, `tcc.h`, `*-gen.c` | TCC 本体 |
| `tcc.vcxproj`, `tcc.sln` | MSVC ビルド |
| `dev/tcc.exe` | 本番配置（`build.bat` がコピー） |
| `dev/include/` | TCC が使う SDK ヘッダ |
| `dev/test/` | smoke / 回帰テスト |
| `test/cppuniut/` | CPPUnit（MSVC） — TCC 駆動ハーネス |
| `test/vs_test/` | CUnit（MSVC） |

---

## 過去に繰り返し起きた破綻 — 必ず予防すること

| パターン | 真因 | 予防策 |
|---------|------|--------|
| **ビルドできなくなる** | コミット前に msbuild を回していない | `pre-commit` / `build.bat` で機械的にゲート |
| **`tccgen.c` が壊れる** | 一括書き換え、関数スコープ崩壊 | 関数単位の最小差分。1 ガイド = 1 コミット |
| **テストが動かなくなる** | 既存 `.c` テストの回帰未検証 | `build.bat` で `con_c_vs_test.exe` を常時実行 |
| **「壊れたから戻す」が乱暴** | `git checkout tccgen.c` で破棄 | branch ごと捨ててタグ `clean-baseline-2026-05-24` から再 branch |

---

## 必須ルール — 規律・コミット

- **1 ガイド = 1 feature branch = 1 コミット**。master で直接作業しない。
- **壊れている状態でコミットしない**。コミット前に必ず `build.bat` が緑。
- **直接復元禁止**：`git checkout <file>` で `tccgen.c` 等を巻き戻さない。branch を捨ててベースラインタグから再開する。
- ベースライン = タグ `clean-baseline-2026-05-24`（無ければ最初に作る）。
- コミットメッセージ例：`feat(cpp): A-1 cpp_mode and extension detection`

---

## 必須ルール — `tccgen.c` の改変

- 関数単位の最小差分のみ。**一括書き換え禁止**。
- 新規変数は **ブロック先頭にまとめて宣言**（MSVC C90 厳格性、`error C2065` の頻発源）。
- **`struct_decl()` 内で `gen_function()` を呼ばない**。`local_stack` / `cur_scope` / `func_vt` / `vtop` が壊れる。
  - メンバ関数本体は `skip_or_save_block(NULL)` で**読み捨て**る（Stage 1）。
- インライン本体保存（`Sym.inline_func_str`）は Stage 3 以降。Stage 1 で接続しない。

---

## 必須ルール — ビルド構成（MSVC）

- **`ONE_SOURCE=0`** を維持。全 `.c`（`tccpp.c`, `tccgen.c`, `tccdbg.c`, `i386-link.c` 等）を個別 `<ClCompile>` で登録。
  - `tcctools.c` は `tcc.c:29` で常時 `#include` のため vcxproj に **追加しない**（追加すると LNK2005）。
- **x86_64 double 6 引数 ABI 修正を保護**：`x86_64-gen.c` の `gfunc_call` で `o(0xc0 + arg*8 + REG_VALUE(d));` の `arg*8` が消えていないこと。
  - 詳細：[履歴.md](履歴.md) 2026-05-23 セクション
  - 回帰テスト `dev/test/repro_double6.c` を `build.bat` に含める（必須）。
- `tccgen.c` を変更したコミットは **Release|x64 と Debug|Win32 の両方**ビルドする。

---

## 必須ルール — ファイル / エンコーディング

- **新規ファイル**（`dev/test/`, テストデータ等）：UTF-8 with BOM 可。
- **既存 TCC ソース**（`tcc.c`, `tccgen.c`, `tccpp.c`, `tcc.h`, `*-gen.c` 等）：**ASCII 維持**。
  - 日本語コメントを書かない（BOM 不要に保つ）。
  - 一括 BOM 化禁止 — diff ノイズになり ONE_SOURCE と相性が悪い。
- ファイル名で改行コード混在を避ける（既存ファイルの LF/CRLF をそのまま維持）。

---

## 必須ルール — C++ 実装（Stage 1）

- C++ 機能は `s1->cpp == 1` のときのみ有効。**`.c` の従来動作を絶対に壊さない**。
- 拡張子 → モード：`.c`/`.h`/`.i` → C、`.cpp`/`.cxx`/`.cc`/`.hpp` → C++、`-x c++` で強制。
- **キーワード降格は `TokenSym.alt_ident_tok` + `tok_alloc_demote()` 方式**（[IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) A-2）。
  - **`tok_alloc()` で降格を試みない** — hash ヒットで `TOK_CLASS` が返り意味がない。
  - **`tok_alloc_new()` で代替しない** — hash チェーン汚染で `.cpp` 側の `TOK_CLASS` 認識が壊れる。
  - 必ず「`tok_alloc_new()` から `*pts = ts;` だけを除いた新ヘルパ」を作る。
- `extern "C"` は **ブロック構文のみ**対応（Stage 1）：
  - `extern "C" { ... }` で `lex_c++/--`（C++ キーワードを識別子化）
  - `extern "C" void foo();` 単一宣言は **Stage 2**
  - `extern "C++" { ... }` は **Stage 2**（Stage 1 では `tcc_error`）
- `.h` ファイルは親 TU の `cpp`/`lex_c` を **継承**（`s1->cpp` リセットされない）。
- 修飾名 `::` および `Class::method` 形式のクラス外定義は **Stage 2 以降**。

---

## 必須ルール — テスト / ビルドフロー

- `build.bat` は次を **errorlevel 0** で通す：
  1. `msbuild tcc.vcxproj /p:Configuration=Release /p:Platform=x64`
  2. `copy /Y x64\Release\tcc.exe dev\tcc.exe`
  3. `dev\tcc.exe -v`
  4. `dev\tcc.exe dev\test\smoke\hello.c -o dev\test\smoke\hello.exe && dev\test\smoke\hello.exe`
  5. **`dev\tcc.exe dev\test\repro_double6.c -o ... && repro_double6.exe`**（ABI 回帰）
  6. `msbuild test\vs_test\con_c_vs_test.vcxproj`（`if exist` スキップ禁止）
  7. `call dev\test\run_all.bat`
- `dev/test/run_all.bat` は必ず先頭で **`pushd "%~dp0"`**。`call` 元のカレントに依存しない。
- `dev/test/aN/` 内のソースは **そのガイド時点の機能だけ**で動くこと（A-2 の `.cpp` は class を使わない等）。
- `mixed_link.bat`（`dev\tcc.exe -c foo.cpp bar.c` の混合 TU テスト）は手動 or `run_all.bat` 末尾で別途呼ぶ。

---

## 着手前に必須の事前 grep

### A-2（キーワードゲート）着手前

```batch
findstr /n /r "\<class\>\|\<this\>\|\<true\>\|\<false\>\|\<namespace\>" *.c *.h
dir /s /b dev\include\*.h | findstr /v \\sample\\
```

または ripgrep があれば：

```batch
rg "\b(class|this|true|false|namespace|operator|virtual|public|private|protected)\b" --glob "*.{c,h}" --glob "!tcctok*.h"
rg "\b(class|this|true|false|namespace|operator|virtual|public|private|protected)\b" dev/include --glob "*.h" --glob "*.hpp"
```

衝突箇所はリスト化してから着手。

### A-4（extern "C"）着手前

`unget_tok(TOK_EXTERN)` の動作と `tokc.str.data` / `tokc.str.size` の構造を 5 行コードで実機検証。

### A-5（class パース）着手前

```batch
findstr /n "basic_type2" tccgen.c
findstr /n "p1 len" tccpp.c
```

`basic_type2:` ラベルの存在、`next_nomacro()` 内の `p1`/`len` スコープを確認。

---

## 失敗時の切り分け表

| 症状 | 最初に確認 | 対処 |
|------|-----------|------|
| LNK2005 重複定義 | `ONE_SOURCE` 未定義 + 全 `.c` 個別コンパイル | `ONE_SOURCE=0` を Preprocessor Definitions に追加 |
| LNK2019 `tccgen_compile` / `tccgen_init` 等 | `tccgen.c` 破損・プレースホルダ化 | **branch 破棄**、タグから再開（`git checkout` 禁止） |
| C4819 / 文字化け | 既存 TCC ソースに BOM 付加していないか | BOM を外す。日本語コメント削除 |
| C2065 `'method_body'` / `'foo'` 未宣言 | 変数宣言がブロック途中（C99 構文）| ブロック先頭にまとめる |
| `.cpp` が bin 扱い | A-1 `guess_filetype()` 未適用 | `libtcc.c` 修正確認 |
| `.cpp` の `class` が syntax error | A-2 降格未適用、または `parse_btype()` に `case TOK_CLASS` がない | A-2 / A-5 の実装確認 |
| `.c` の `int class;` が syntax error | A-2 `effective_cpp_lex()` / `tok_alloc_demote()` が動いていない | `int class; class=2;` で 1 シンボル解決を smoke |
| `int class;` が動くが `class++;` で型不一致 | `alt_ident_tok` キャッシュが効いていない（毎回新 ID） | `TokenSym.alt_ident_tok` フィールドが追加されているか確認 |
| `libtcc1-64.a not found` | `dev/lib/` の配置と `tcc.h` の `TCC_LIBTCC1` 分岐 | [履歴.md](履歴.md) 2026-05-24 セクション参照 |
| double 6 引数の glOrtho 等が壊れる | x86_64-gen.c の `arg*8` 修正消失 | [履歴.md](履歴.md) 2026-05-23 + `repro_double6.c` で確認 |

---

## Windows / PowerShell 操作ルール

（`.github/copilot-instructions.md` と同一）

- **実行ファイル**：カレントディレクトリのバイナリ実行には必ず `./` または `.\` を付ける。
- **バイナリ解析**：自作スクリプトを避け `certutil -encodehex <file> out.hex` を使う。
- **クォート**：`powershell -Command` での複雑なエスケープを避ける。
- **コマンド対応**：
  - `ls` → `dir`
  - `grep` → `findstr`
  - `rm -rf` → `Remove-Item -Recurse`
  - `head` → `gc -Head`
- **Python を使わない**（環境にインストールされていない）。

---

## 暗黙でやらないこと

- master への直接コミット
- 複数ガイドを 1 コミットにまとめる
- `git checkout <file>` で破損ファイルを「元に戻す」
- `--no-verify` での pre-commit バイパス
- 既存 TCC ソースへの日本語コメント追加
- `tcctok.h` を分割して enum から C++ キーワードを除外する（v2.2 で否定済み、ハイブリッド方式に確定）
- ファイル毎の `register_cpp_keywords()` 遅延登録（v2.2 で否定済み、混合 TU で破綻）
- `force_alloc_identifier()` 毎回呼び出し（v2.3 で否定済み、同名 sym 不一致）
- `tok_alloc(name, len)` を降格に使う（v2.4 で否定済み、hash ヒットで TOK_CLASS が返る）

---

## 参考ドキュメント

| 文書 | 内容 |
|------|------|
| [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) | C++ 実装の詳細プラン（A-P/A-0〜A-11、Part B 付録） |
| [PLAN.md](PLAN.md) / [TCC_CPP.md](TCC_CPP.md) | 上位プラン |
| [履歴.md](履歴.md) | 過去の修正履歴（ビルドセットアップ、x86_64 ABI バグ、class 着手記録） |
| [問題と原因.md](問題と原因.md) | 失敗事例の自己分析と次プラン |
| [実装前に読む.md](実装前に読む.md) | レビュー時の注意点インデックス |
| [レビューその4.md](レビューその4.md)〜[その8.md](レビューその8.md) | IMPLEMENTATION_GUIDE の改訂レビュー履歴 |
