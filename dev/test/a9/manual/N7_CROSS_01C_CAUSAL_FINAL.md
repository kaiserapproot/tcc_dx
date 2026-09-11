# N7-CROSS-01C causal isolation final report

Authority base: `88c5df4`. Production fix: `cpp_global_copy_init_decl_allowed()`.

## Isolation matrix (measured on fixed binary + 88c5df4 baseline)

| Case | Result (88c5df4) | Result (fix) |
|------|------------------|--------------|
| A primary TU copy-init | PASS | PASS |
| B included header copy-init | FAIL (constant init) | PASS |
| C -E flatten (.cpp) | PASS | PASS |

## Diagnostic gate trace (TCC_N7_CROSS_01C_DIAG=1)

| Case | CPP_IN_USER_SOURCE_FILE | FILE_PREV_PRESENT | DECL_ALLOWED | INTERCEPTED |
|------|-------------------------|-------------------|--------------|-------------|
| A primary | 1 | 0 | 1 | 1 |
| B header | 0 | 1 | 1 | 1 |
| C -E flat | 1 | 0 | 1 | 1 |

Pattern on 88c5df4 (before fix): B had `CPP_IN_USER_SOURCE_FILE=0` and
`GLOBAL_COPY_INIT_INTERCEPTED=0` because `cpp_in_user_source_file()` required
`file->prev == NULL`.

## Root cause

```
DIRECT_CAUSE=N7_CROSS_01B_PRIMARY_TU_ONLY_GATE
ROOT_CAUSE=Global copy-init gate used cpp_in_user_source_file() (primary TU
  only), rejecting real #include file declarations.
ROOT_CAUSE_CONFIRMED=YES
```

## Fix (01C-B)

- New `cpp_global_copy_init_decl_allowed()`: allow real primary and #include
  paths; reject `:global_copy_init:` / `:inline:` synthetic replay; block
  reentry when `cpp_global_copy_init_emit_ent` is active.
- FEAT-4F/4G gates unchanged (`cpp_in_user_source_file()` untouched).

## Scope breadth (header context, out of 01C scope)

| Form | Result |
|------|--------|
| H1 copy-init | PASS (01C fix) |
| H2 default ctor | OUT_OF_SCOPE (FEAT-4G primary-only) |
| H3 direct ctor | OUT_OF_SCOPE |
| H4 class array | OUT_OF_SCOPE |

## Safety

| Check | Result |
|-------|--------|
| WINAPI `#include <windows.h>` | PASS, no crash |
| Reentrant registration | NO (count=1) |
| 01B hardening regression | PASS |

## cross.h

Not present in this repository; compile deferred to Amateras retest.
