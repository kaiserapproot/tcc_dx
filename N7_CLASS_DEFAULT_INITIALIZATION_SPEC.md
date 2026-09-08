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

`dev/test/a9/manual/n7_00_measure.bat` — records current behavior; exit 0 when measurement completes.

## POST-N6 linkage

```text
POST_N6_00=COMPLETE
POST_N6_00_CLOSURE_COMMIT=b229b4c
RECOMMENDED_N7=CLASS_DEFAULT_INITIALIZATION
CONFIRMED_SILENT_CASE=class P { P(int); }; P f; (local auto)
POST_N6_INVENTORY_STANDALONE=YES
RUN_ALL_INTEGRATION=DEFERRED
```
