# N7 — Class Default Initialization / Default-Constructor Viability

> Authority for N7-00 spec freeze. **Production change: NONE** until N7-01 review.

## Theme

**N7=CLASS_DEFAULT_INITIALIZATION** (not merely "implicit default ctor synthesis").

When `class P { public: P(int); };` and `P f;`:

- ISO semantics: **no viable default constructor** → **compile error**
- Current TCC (Post-N6): **compile pass**, **no construction** → **silent miscompile** (SA-01)

N7-01 first goal: **fail-closed before codegen expansion** (`N7_01_SILENT_MISCOMPILE_ELIMINATION`).

## Roadmap

| Phase | Scope | Production |
|-------|--------|------------|
| N7-00 | Capability / semantics freeze + measurement | NONE |
| N7-01 | No-viable-default-constructor → diagnostic + compile fail | YES |
| N7-02 | Implicit default ctor codegen (trivial / base / member) | YES |
| N7-03 | Array default-construction propagation | YES |
| N7-04 | Storage-class regression (global / static / TLS / local-auto) | audit only in N7-00 |
| N7-05 | Implicit-special-member interaction audit | TBD |
| N7-06 | Negative / fail-closed qualification | TBD |
| N7-07 | Full regression / closure | TBD |

## N7-00 probe matrix

| ID | Case | Expected (ISO) |
|----|------|----------------|
| A | `struct A { int x; }; A a;` | implicit default ctor available; object initialized |
| B | `struct A { A(); }; A a;` | user default ctor called once |
| C | `struct A { A(int); }; A a;` (local auto) | **COMPILE_FAIL** no viable default ctor |
| D | member `M(int)` in `A` | **COMPILE_FAIL** if implicit default ctor ill-formed |
| E | base `B(int)`, `D : B` | **COMPILE_FAIL** if implicit default ctor ill-formed |
| F | `A a[4]` with `A(int)` only | **COMPILE_FAIL** (array propagation) |

### Storage-class split (case C shape)

Same `A(int)` only — measure separately:

- LOCAL_AUTO
- GLOBAL
- LOCAL_STATIC
- THREAD_LOCAL (N6 may already fail-closed)

**N7-00 measured behavior (2026-09-08, `n7_00_measure.bat`):**

| Probe | Result |
|-------|--------|
| A trivial member | COMPILE_PASS_RUN_PASS |
| B user `A()` | COMPILE_PASS_RUN_PASS |
| C local auto `P(int)` only | **SILENT_MISCOMPILE** |
| D member no default | COMPILE_FAIL_CLOSED |
| E base no default | COMPILE_FAIL_CLOSED |
| F array | **SILENT_MISCOMPILE** |
| GLOBAL | **SILENT_MISCOMPILE** |
| LOCAL_STATIC | **SILENT_MISCOMPILE** |
| THREAD_LOCAL | COMPILE_FAIL_CLOSED (N6) |

`BUG_SCOPE=SHARED_DEFAULT_INITIALIZATION_CHECK` — TLS path already fail-closed; N7-01 must cover auto/global/static/array without altering N5/N6 semantics.

## Gate

| Gate | Role |
|------|------|
| `dev/test/a9/manual/n7_00_measure.bat` | N7-00 freeze measurement |
| `dev/test/a9/manual/n7_01_default_ctor_fail_closed.bat` | N7-01 fail-closed authority (in `run_all.bat`) |

## N7-01 closure authority

```text
=== N7-01 FINAL ===
N7_00_COMMIT=bf5a2a3
N7_01_IMPL_COMMIT=ba22114
N7_01_CLOSURE_COMMIT=882891d
N7_01_MERGE_COMMIT=45fc17b

BUG_SCOPE=SHARED_DEFAULT_INITIALIZATION_CHECK
NO_VIABLE_DEFAULT_CTOR_AUTHORITY=cpp_validate_decl_default_initialization
DEFAULT_CTOR_VIABILITY_CHECK_STAGE=decl() fallback before decl_initializer_alloc()
ZERO_ARGUMENT_CONSTRUCTION_IS_VIABLE=cpp_class_has_default_ctor (via cpp_ctor_viable_with_zero_args)

LOCAL_AUTO_NO_DEFAULT_CTOR=FAIL_CLOSED
ARRAY_NO_DEFAULT_CTOR=FAIL_CLOSED
GLOBAL_NO_DEFAULT_CTOR=FAIL_CLOSED
LOCAL_STATIC_NO_DEFAULT_CTOR=FAIL_CLOSED

N7_00_SILENT_MISCOMPILE_COUNT=4
N7_01_SILENT_MISCOMPILE_COUNT=0

DEFAULT_ARGUMENT_CTOR=PASS
OVERLOADED_WITH_DEFAULT_CTOR=PASS
TRIVIAL_ARRAY_DEFAULT_INIT=PASS

RUNTIME_CHANGE=NONE
PUBLIC_API_CHANGE=NONE
N7_01=PASS
N7_01=COMPLETE
FULL_GATE_AT_N7_01_CLOSURE_COMMIT=PASS
N7_01_MASTER_GATE=PASS
RUN_ALL_GATING_FAILURES=0
RUN_ALL_CRASHES=0
N7_02_START=NO
```

## POST-N6 linkage

```text
POST_N6_00=COMPLETE
POST_N6_00_CLOSURE_COMMIT=b229b4c
RECOMMENDED_N7=CLASS_DEFAULT_INITIALIZATION
CONFIRMED_SILENT_CASE=class P { P(int); }; P f; (local auto)
POST_N6_INVENTORY_STANDALONE=YES
RUN_ALL_INTEGRATION=DEFERRED
```
