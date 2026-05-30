---
name: tcc-cpp
description: >-
  Implements and maintains C++98 subset extensions in Tiny C Compiler (TCC) on
  MSVC/Windows x64. Use when editing tccgen.c, tccpp.c, tcc.h, *-gen.c, adding
  C++ features (class, this, mangling, inheritance, virtual), running build.bat,
  or debugging TCC C++ regressions in this repository.
---

# TCC C++98 Extension (tcc_dx)

Tiny C Compiler に C++98 サブセットを段階的に実装するプロジェクト。DirectX 等の OOP API を自然に扱うことが目的。

## Quick Start

C++ 実装・修正に着手する前に必ず確認:

1. **現在の到達状況**: [実装済み.md](実装済み.md) と [次の実装.md](次の実装.md)
2. **詳細手順**: [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)（A-0〜A-11）
3. **上位プラン**: [TCC_CPP.md](TCC_CPP.md)（6 Stage ロードマップ）
4. **失敗事例**: [履歴.md](履歴.md) / [問題と原因.md](問題と原因.md)

## Project Layout

| Path | Role |
|------|------|
| `tcc*.c`, `tcc.h`, `*-gen.c` | TCC core |
| `tcc.vcxproj` | MSVC build (`ONE_SOURCE=0`) |
| `dev/tcc.exe` | Production binary (`build.bat` copies here) |
| `dev/include/` | SDK headers for TCC |
| `dev/test/` | Smoke / regression tests (`run_all.bat`) |
| `test/vs_test/` | CUnit (MSVC) |
| `test/cppuniut/` | CPPUnit harness |

## Mandatory Discipline

### Branch / Commit

- **1 guide = 1 feature branch = 1 commit**. Never work directly on `master`.
- **Never commit broken state**. `build.bat` must pass before commit.
- **Never `git checkout <file>` to restore** broken `tccgen.c`. Discard branch, restart from tag `clean-baseline-2026-05-24`.
- Commit message example: `feat(cpp): A-1 cpp_mode and extension detection`

### tccgen.c Rules

- **Minimal diff per function only**. No bulk rewrites.
- **Declare new variables at block top** (MSVC C90 strict — avoids `error C2065`).
- **Never call `gen_function()` inside `struct_decl()`** — corrupts `local_stack` / `cur_scope` / `func_vt` / `vtop`.
- Stage 1: member bodies via `skip_or_save_block(NULL)` (discard).
- Stage 3+: inline body save via `Sym.inline_func_str` + delayed compile at `};`.

### Build (MSVC)

- Keep **`ONE_SOURCE=0`**. All `.c` files as separate `<ClCompile>`.
- **Do NOT add `tcctools.c`** to vcxproj (included from `tcc.c:29` → LNK2005).
- **Protect x86_64 double-6-arg ABI**: `x86_64-gen.c` `gfunc_call` must keep `o(0xc0 + arg*8 + REG_VALUE(d));`.
- After `tccgen.c` changes: build **Release|x64 AND Debug|Win32**.

### Encoding

- **Existing TCC sources**: ASCII only, no Japanese comments, no BOM.
- **New test files** (`dev/test/`): UTF-8 with BOM OK.

## C++ Implementation Rules

All C++ features gated on **`s1->cpp == 1`**. **Never break `.c` behavior.**

| Rule | Detail |
|------|--------|
| Extension → mode | `.c`/`.h`/`.i` → C; `.cpp`/`.cxx`/`.cc`/`.hpp` → C++; `-x c++` forces |
| Keyword demotion | `TokenSym.alt_ident_tok` + `tok_alloc_demote()` only |
| Never use | `tok_alloc()` for demotion (hash hit returns `TOK_CLASS`) |
| Never use | `tok_alloc_new()` substitute (pollutes hash chain) |
| `.h` inheritance | Parent TU's `cpp`/`lex_c` preserved |
| `extern "C" { ... }` | Implemented |
| `extern "C" void foo();` | Implemented (`decl_once_flag`) |
| `extern "C++" { ... }` | Not supported → `tcc_error` |
| Qualified names `::` | Implemented (Stage 2) |

## Implementation Stages (Roadmap)

From [TCC_CPP.md](TCC_CPP.md). Check [実装済み.md](実装済み.md) for current status.

| Stage | Focus |
|-------|-------|
| 1 | Lexer, keywords, `class` as `struct` alias (default `private`) |
| 2 | Name mangling, overload, `&`, `::`, `Sym.parent_class` |
| 3 | Member functions, `this`, `obj.method()`, default args, delayed inline parse |
| 4 | Single inheritance, access control, ctor/dtor, init lists, static members |
| 5 | Virtual functions, VTable (`_vptr`), indirect calls |
| 6 | Operator overload, `const` member functions |

**Not in scope (initial)**: exceptions, RTTI, templates, `nullptr` (C++11).

## Core Architecture

### Key Files

| File | Role |
|------|------|
| `tccpp.c` | Tokenizer / preprocessor (`next_nomacro`, `next`, `skip_or_save_block`) |
| `tcctok.h` | Token definitions (`DEF()` macro) |
| `tcc.h` | `TokenString`, `TokenSym`, `Sym`, `CType` |
| `tccgen.c` | Parser + codegen (`decl`, `struct_decl`, `unary`, `gen_function`) |
| `x86_64-gen.c` | x64 ABI (`gfunc_call`, `gfunc_prolog`) |

### Delayed Parsing (1-pass workaround)

1. Register prototype early during class parse.
2. Save body tokens via `skip_or_save_block` → `Sym` / `TokenString`.
3. Finish class layout at `};`.
4. Replay saved tokens and call `gen_function` **outside** `struct_decl`.

### this Pointer ABI (Windows x64)

- Non-static member: `this` as **1st arg in RCX**.
- `obj.method()` → `Class::method(&obj)`.
- Configure in `gfunc_prolog` / `gfunc_call` in `x86_64-gen.c`.

### Parser Touch Points

- `struct_decl()`: class parse, access specifiers, member registration.
- `unary()`: `obj.method()` / `->` → hidden `this` on `vtop`.
- `decl()`: name mangling before symbol registration.
- `type_decl()`: references, default args, member pointer types.

## Pre-Implementation Grep

Run before starting relevant guides:

**A-2 (keyword gate)**:
```batch
findstr /n /r "\<class\>\|\<this\>\|\<true\>\|\<false\>\|\<namespace\>" *.c *.h
dir /s /b dev\include\*.h | findstr /v \\sample\\
```

**A-5 (class parse)**:
```batch
findstr /n "basic_type2" tccgen.c
findstr /n "p1 len" tccpp.c
```

## Build & Test Workflow

`build.bat` must pass all steps (errorlevel 0):

```
1. msbuild tcc.vcxproj /p:Configuration=Release /p:Platform=x64
2. copy x64\Release\tcc.exe dev\tcc.exe
3. dev\tcc.exe -v
4. dev\tcc.exe dev\test\smoke\hello.c → run hello.exe
5. dev\tcc.exe dev\test\repro_double6.c → run (ABI regression)
6. msbuild test\vs_test\con_c_vs_test.vcxproj
7. call dev\test\run_all.bat
```

- `run_all.bat` must start with `pushd "%~dp0"`.
- Tests in `dev/test/aN/` use **only features available at that guide stage**.
- New C++ test: add minimal `.cpp` under appropriate `aN/`, verify with `.\dev\tcc.exe`.

## Implementation Workflow

When adding a C++ feature:

```
Task Progress:
- [ ] Read 実装済み.md — confirm not already done
- [ ] Read IMPLEMENTATION_GUIDE.md section (A-N)
- [ ] Run pre-implementation grep for that guide
- [ ] Create feature branch from clean baseline
- [ ] Minimal tccgen.c / tccpp.c diff (function-scoped)
- [ ] Add dev/test/aN/*.cpp smoke test
- [ ] Run build.bat (full gate)
- [ ] Update 実装済み.md if feature complete
- [ ] Single commit on feature branch
```

## Troubleshooting

| Symptom | First Check | Fix |
|---------|-------------|-----|
| LNK2005 duplicate | `ONE_SOURCE` + vcxproj | Set `ONE_SOURCE=0`, all `.c` separate |
| LNK2019 tccgen_* missing | `tccgen.c` corrupted | Discard branch, restart from tag |
| C4819 / garbled text | BOM in TCC sources | Remove BOM, no Japanese in `.c` |
| C2065 undeclared var | C99 mid-block declare | Move declarations to block top |
| `.cpp` treated as binary | `guess_filetype()` | Fix `libtcc.c` |
| `.cpp` `class` syntax error | A-2/A-5 | Demotion + `parse_btype` `TOK_CLASS` |
| `.c` `int class;` fails | Demotion off | `effective_cpp_lex()` / `tok_alloc_demote()` |
| `class++` type mismatch | `alt_ident_tok` cache | Verify `TokenSym.alt_ident_tok` field |
| double 6-arg broken | `arg*8` in x86_64-gen.c | Restore + run `repro_double6.c` |
| libtcc1-64.a not found | `dev/lib/` layout | See 履歴.md 2026-05-24 |

## Never Do

- Commit on master directly
- Multiple guides in one commit
- `git checkout <file>` to "fix" broken sources
- `--no-verify` on pre-commit
- Japanese comments in existing TCC sources
- Split `tcctok.h` enum for C++ keywords (rejected v2.2)
- Per-file `register_cpp_keywords()` (rejected v2.2)
- `force_alloc_identifier()` every call (rejected v2.3)
- `tok_alloc(name, len)` for keyword demotion (rejected v2.4)

## Windows / PowerShell

- Run local binaries: `.\dev\tcc.exe` (not bare `dev\tcc.exe`)
- Prefer `findstr` over `grep`, `dir` over `ls`
- No Python (not installed)
- Avoid complex `powershell -Command` quoting
- Binary inspect: `certutil -encodehex <file> out.hex`

## Additional Resources

- Stage details, tokenizer internals, member pointer plan: [reference.md](reference.md)
- Detailed step-by-step guides: [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md)
- Past fixes and ABI notes: [履歴.md](履歴.md)
