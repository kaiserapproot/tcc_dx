# TCC C++ Reference (from TCC_CPP.md)

Progressive disclosure supplement. Read only when implementing specific stages.

## Stage Details

### Stage 1 — Lexer & Basic Types

- Register: `class`, `public`, `protected`, `private`, `virtual`, `this`, `operator`, `bool`, `true`, `false`, `namespace`.
- `class` = `struct` alias with default access `private`.
- `nullptr` excluded (C++11); optional later extension.

### Stage 2 — Name Mangling & Scope

- Function overloading by argument types.
- Lightweight internal mangling (`__tcc_<name>_<args>`), not full Itanium/MSVC compat.
- `Sym.parent_class` for class-scope lookup.
- Reference type `&` in parse and mangling.

### Stage 3 — Member Functions & this

- Parse class member definitions.
- Implicit `this` as 1st parameter in `gfunc_prolog`.
- `a.func()` → `Class::func(&a)`.
- Default arguments: save at declaration, apply at call site.

### Stage 4 — Inheritance & Access Control

- Single inheritance: base struct at derived head.
- `public`/`protected`/`private` symbol lookup errors.
- Implicit ctor/dtor for stack objects.
- Member initializer lists: `Class() : m1(0), m2(1) {}`.
- Static member vars/funcs → data section.

### Stage 5 — Virtual Functions

- `_vptr` at class head.
- VTable in `.rodata`.
- Indirect call via `_vptr`, not direct jump.

### Stage 6 — Operator Overload & const

- `operator+`, `operator[]` → mangled function calls.
- `const` member → const-correct `this` pointer checks.

## Delayed Parsing Mechanism

```
Class parse:
  prototype seen → register Sym early
  body { ... }   → skip_or_save_block → TokenString on Sym
  }; reached     → layout finalized
                 → replay each saved body → gen_function (NOT inside struct_decl)
```

Resolves forward references within class without abandoning 1-pass design.

## Tokenizer Internals

| Concept | Detail |
|---------|--------|
| `tok` | Current token ID; 1-255 ASCII; 256+ keywords from `tcctok.h` |
| `tokc` | `CValue` union for literals |
| `next_nomacro()` | Core lex: skip WS/comments, identifiers via `tok_alloc()` |
| `next()` | Macro-aware; reads from `TokenString` if in macro expansion |
| `TokenString` | Dynamic `int[]` of token IDs + embedded data for `tokc` tokens |
| `skip_or_save_block` | Brace-match save/discard for inline member bodies |

## Parser Hierarchy (tccgen.c)

```
decl() → parse_btype(), type_decl(), gen_function()
block() → statements, recursive on nested {}
gexpr() → expr_eq() → ... → unary()  (deepest: calls, literals, identifiers)
```

**vtop**: virtual value stack top; expression results during parse/codegen.

## ABI: this Pointer by Platform

| Platform | this register |
|----------|---------------|
| Windows x64 | RCX (1st arg) |
| x86 Windows | ECX (`__thiscall` if needed) |
| ARM 32 (AAPCS) | R0 (watch sret shift to R1) |
| ARM64 (AAPCS64) | X0; 16-byte stack alignment strict |

## Member Pointer Plan (Future)

**Phase 1 — Data member pointer** (`T C::*`):
- Internal: offset as int.
- `obj.*p` → `*(T*)((char*)obj + offset)`.

**Phase 2 — Non-virtual PMF** (single inheritance only):
- Internal: cast to `R(*)(void*, Args...)`.
- `(obj.*pmf)(args)` → wrapper call with `&obj`.

**Not planned initially**: virtual PMF, multiple inheritance adjustor thunks.

## Test Matrix

| Test | Criterion |
|------|-----------|
| Name mangling | `f(int)` vs `f(double)` → distinct symbols |
| this pass | Member access uses correct RCX |
| VTable lookup | Base ptr calls derived override |
| Access control | External private access → `tcc_error` |
| Default args | Omitted args get default values on stack/regs |

## Explicitly Out of Scope

- Exception handling (`try`/`catch`/`throw`)
- RTTI (`dynamic_cast`, `typeid`)
- Templates
- Full MSVC/Itanium mangling compatibility
