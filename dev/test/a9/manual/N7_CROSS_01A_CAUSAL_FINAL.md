# N7-CROSS-01A causal isolation final report

Authority commit: `7509dbe`. Production change: **none**.

## Isolation matrix (measured)

| Case | File | Result |
|------|------|--------|
| A scalar constant | `n7_cross_01a_a_scalar.cpp` | PASS |
| B class brace init | `n7_cross_01a_b_class_const.cpp` | PASS |
| C local `V v = make_v()` | `n7_cross_01a_c_local_func.cpp` | PASS |
| D global `V g = make_v()` | `n7_cross_01a_d_global_func.cpp` | COMPILE_FAIL |
| E global `const V g = make_v()` | `n7_cross_01a_e_global_const.cpp` | COMPILE_FAIL (same as D) |
| Amateras repro A | `n7_00_minimal_static_dyninit.cpp` | COMPILE_FAIL |
| Amateras repro B | `n7_cross_01a_minimal_dyninit.cpp` | COMPILE_FAIL |
| N7-02 control | `n7_cross_01a_f_global_default.cpp` | PASS |

D and E fail at the same site with the same diagnostic (Japanese message
decodes to **initializer element is not constant**).

## Pipeline trace

| Stage | Global `= make_v()` | Local `V v = make_v()` |
|-------|---------------------|-------------------------|
| Declaration parse | YES | YES |
| Initializer parse | `decl_initializer` + `parse_init_elem(EXPR_CONST)` | FEAT-COPY-INIT: `expr_eq()` at runtime |
| Constant init required | YES (`p->sec` set, `DATA_ONLY_WANTED`) | NO |
| Dynamic init registered | NO | N/A (stack) |
| Startup `.init_array` | NO | N/A |

## N7-02 machinery comparison

| Form | Path | Status |
|------|------|--------|
| `static V g;` | FEAT-4G -> `cpp_register_global_dyn` -> `.init_array` | WORKS |
| `static V g(args);` | FEAT-4G ctor-call form | WORKS (when args are type names) |
| `static V g = make_v();` | `decl_initializer_alloc(has_init=1)` -> EXPR_CONST | **FAIL** |

Existing global dyn machinery evaluates **ctor calls** saved as tokens.
It does **not** handle copy-init `= expr` with a runtime function call.

Local copy-init (`tccgen.c` FEAT-COPY-INIT ~18854) already evaluates
`expr` at runtime and materializes into the object. Global scope lacks
the analogue.

## First rejection site

```
FIRST_REJECTION_FUNCTION=parse_init_elem
FIRST_REJECTION_FILE=tccgen.c
FIRST_REJECTION_LINE=17112
FIRST_REJECTION_CONDITION=EXPR_CONST: vtop after expr_const1() is not VT_CONST
```

Function call `make_v()` is parsed by `expr_const1()` but rejected because
global static storage initializers require compile-time constants.

## Dependency separation

| Dependency | Verdict |
|------------|---------|
| RETURN_BY_VALUE | REJECTED (local PASS) |
| COPY_CTOR | REJECTED as root (local copy-init works) |
| COPY_ASSIGN | NO (never degraded to `g = make_v()` assignment) |
| CONST | REJECTED (D and E identical failure) |
| GLOBAL_SCOPE | CONFIRMED |

## Root cause

```
DIRECT_CAUSE=Global copy-init `T obj = expr` uses C static initializer
  path (EXPR_CONST) and rejects runtime function-call expressions.

ROOT_CAUSE=Missing global dynamic initialization registration for
  non-constant copy-init initializers. N7-02 startup machinery covers
  implicit/explicit ctor forms only, not `= make_object()`.
```

## Amateras signature

```
static const mmd_dq_t c_identity_dq = mmd_dq_make_identity();
```

Matches repro B shape. `MIN_REPRO_SIGNATURE_MATCH_AMATERAS=YES`.

## Regression (unchanged binary)

```
N7_02_REGRESSION=PASS
N7_01_REGRESSION=PASS (via prior gates)
CRASH_COUNT=0
```

## Next phase

```
ROOT_CAUSE_CONFIRMED=YES
N7_CROSS_01B_FIX_START=YES
```

N7-CROSS-01B must add global-scope runtime initializer evaluation and
startup registration **without** reusing N7-02 implicit-default-ctor
machinery as a bolt-on, and **without** degrading to copy assignment.
