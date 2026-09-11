# N7-07D STATIC LOCAL CLASS ARRAY FAIL-CLOSED — TCC FINAL REPORT

```text
=== TCC N7-07D STATIC LOCAL CLASS ARRAY FAIL-CLOSED FINAL ===

BASE_HEAD=0827d03

N7_07D_TEST_COMMIT=(pending)
N7_07D_FIX_COMMIT=(pending)

TCC_PRODUCTION_CHANGE=YES

STATIC_LOCAL_SCALAR_COMPILE=PASS
STATIC_LOCAL_SCALAR_RUNTIME=PASS
STATIC_LOCAL_SCALAR_CTOR_COUNT=1
STATIC_LOCAL_SCALAR_INIT_ONCE=PASS

STATIC_LOCAL_CLASS_ARRAY_BEFORE_CTOR_COUNT=0

DIRECT_CAUSE=decl() FEAT-4F gate requires VT_STRUCT scalar; static local arrays fall through with storage but no init-once ctor walker
ROOT_CAUSE=cpp_validate_local_automatic_class_array and cpp_emit_local_array_default_ctor_calls exclude VT_STATIC; FEAT-4F scalar init-once path excludes VT_ARRAY
ROOT_CAUSE_CONFIRMED=YES

STATIC_LOCAL_CLASS_ARRAY_COMPILE=FAIL
STATIC_LOCAL_CLASS_ARRAY_DIAGNOSTIC=PASS
STATIC_LOCAL_CLASS_ARRAY_FAIL_CLOSED=PASS

STATIC_LOCAL_MULTIDIM_CLASS_ARRAY_COMPILE=FAIL
STATIC_LOCAL_MULTIDIM_DIAGNOSTIC=PASS

TRIVIAL_STATIC_LOCAL_STRUCT_ARRAY=PASS

AUTOMATIC_LOCAL_CLASS_ARRAY=PASS
AUTOMATIC_LOCAL_CLASS_ARRAY_CTOR_COUNT=4

GLOBAL_CLASS_ARRAY=PASS
CLASS_MEMBER_ARRAY=PASS

STATIC_LOCAL_ARRAY_EXPLICIT_INIT_BEFORE=COMPILE_FAIL (T() value-init unsupported; separate deferred form)

N7_07C_LOCAL_ARRAY_REGRESSION=PASS
N7_07C_EXTERN_C_REGRESSION=PASS
N7_07B_REGRESSION=PASS

BAD_CODE_ACCEPTED=0
SILENT_MISCOMPILE_COUNT=0
COMPILER_CRASH_COUNT=0

BUILD_BAT=PASS
RUN_ALL=PASS
RUN_ALL_GATING_FAILURES=0
RUN_ALL_CRASHES=0

N7_07D_TCC_STATUS=CLOSED
READY_FOR_AMATERAS_N7_07D_AUDIT=YES
```

## Causal isolation (pre-fix, dev/tcc.exe @ 0827d03)

| Case | Compile | Runtime / ctor count |
|------|---------|----------------------|
| A: `static M m;` x3 calls | PASS | CTOR_COUNT=1 (init-once) |
| B: `static M a[4];` x3 calls | PASS | CTOR_COUNT=0 (silent miscompile) |
| C: `M a[4];` | PASS | CTOR_COUNT=4 |
| D: `M g[4];` global | PASS | CTOR_COUNT=4 |

## Production change

`tccgen.c`: `cpp_validate_local_static_class_array()` hooked in `decl()` for
function-local static arrays without explicit initializer whose elements need
default ctor emission. Mirrors N7-07C automatic guard predicates; array-only.

## Follow-up

- `n7_03_amateras_bone_key.cpp`: reverted `static store[2]` to automatic
  `store[2]` (N7-07C path) so copy-assign min-repro stays green under N7-07D.
- Explicit init `static M a[2] = { M(), M() }` remains compile-fail (separate form).
- N7-07E (init-once static array construction) deferred until Amateras audit.
