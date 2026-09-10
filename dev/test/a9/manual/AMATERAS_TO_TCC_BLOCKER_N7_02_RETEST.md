# Amateras -> TCC blocker report (N7-02 consumer retest)

## Context

- TCC_COMMIT: 5ade5dc (pre-fix baseline for this report)
- Amateras retest stopped at vec_quat; bone / cross.h not attempted.

## Observed failure

`vec3 b = a * 2.0f;` (vec_quat_matrix_cpp.cpp:48) fails to compile when
`operator*(V&)` is declared before `operator*(float)`.

| Case | Result |
|------|--------|
| `a * 2.0f` (vec_quat shape) | FAIL |
| `a.operator*(2.0f)` | PASS |
| declare `operator*(float)` first | PASS |

Root cause: first arity-matching overload by declaration order; no argument
type scoring on implicit member binop.

## TCC fix scope (commit A)

- `cpp_try_member_binop`: use `cpp_score_member_overloads` (same scoring as
  member call resolution).

## Authority repro

`dev/test/a9/manual/n7_02_retest_binop_scalar_ovl.cpp`

NEXT_TCC_BLOCKER=MEMBER_BINOP_OVERLOAD_RESOLUTION_BY_ARG_TYPE
