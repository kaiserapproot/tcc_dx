# N7-02-02 Preflight B — Destructor Strategy Audit

> Read-only audit @ master merge `7f3ab14`. No production change in this document.

## Question

When implicit outer `D d;` gains synthetic `D::D()` (Design A ctor), who emits
`M::~M()` / `B::~B()` at scope exit / global fini / local-static teardown?

## Path comparison

| Storage | Ctor path today | Dtor path today | Requires user `~D()`? |
|---------|-----------------|-----------------|----------------------|
| **LOCAL_AUTO** | FEAT-4F `d.D()` rewrite | `cpp_finish_scope` -> `cpp_call_scope_dtors` -> `cpp_emit_local_dtor` | **YES** (`cpp_find_dtor_field`) |
| **GLOBAL** | FEAT-4G `.init_array` via `cpp_register_global_dyn` | `cpp_register_global_dyn` fini entry when `cpp_find_dtor_field` | **YES** |
| **LOCAL_STATIC** | guarded FEAT-4F + once init | `cpp_prepare_local_static_dtor` -> wrapper thunk | **YES** (else fail-closed @ 2386-2387) |
| **USER ~D()** | N/A | `gen_function()` epilog: `cpp_emit_member_dtor_calls` then `cpp_emit_base_dtor_calls` | user symbol |

### Authority functions

```text
USER_OUTER_DTOR_PATH=gen_function() @ tccgen.c:17773-17779
LOCAL_AUTO_IMPLICIT_DTOR_PATH=cpp_emit_local_dtor() @ 1565-1598
LOCAL_STATIC_IMPLICIT_DTOR_PATH=cpp_prepare_local_static_dtor() @ 2369-2397
GLOBAL_IMPLICIT_DTOR_PATH=cpp_register_global_dyn(is_dtor=1) @ 2614-2623
SUBOBJECT_DTOR_EMIT_HELPERS=cpp_emit_member_dtor_calls()+cpp_emit_base_dtor_calls()
SUBOBJECT_DTOR_REQUIRES_THIS=cpp_this_sym (4755, 4016)
```

### Current implicit-outer behavior (no user `~D()`)

| Site | Behavior |
|------|----------|
| `cpp_emit_local_dtor` | `cpp_validate_implicit_dtor(class,0)` then **return without emit** (1580-1582) |
| `cpp_prepare_local_static_dtor` | if `cpp_class_requires_destruction` -> **tcc_error** implicit dtor unsupported (2386-2387) |
| `cpp_register_global_dyn` (ctor reg) | `cpp_validate_implicit_dtor` if no user ~D; **no fini entry** unless `cpp_find_dtor_field` (2614-2622) |
| `cpp_scope_has_local_dtor_between` | only counts objects with **user** `~D()` (1918-1920) |

`cpp_class_requires_destruction()` @ 4409-4447 **does** walk members/bases and returns 1 when subobjects need destruction — but no emitter consumes this for implicit outer today.

## Design options

### DTOR_DESIGN_A — synthetic `~D()` symbol

Mirror Design A ctor:

1. At `struct_decl()` end, if class has no user `~D()` and `cpp_class_requires_destruction(class)`:
   inject member-field `~D()` with empty body `{}`.
2. `cpp_finish_member_inlines()` registers mangled global dtor.
3. Existing hooks call it:
   - local: `cpp_emit_local_dtor` -> `~D(&obj)`
   - global: `cpp_register_global_dyn` fini wrapper
   - local-static: `cpp_prepare_local_static_dtor`
4. `gen_function()` dtor epilog runs `cpp_emit_member_dtor_calls` / `cpp_emit_base_dtor_calls`.

| Criterion | Assessment |
|-----------|------------|
| exactly once | **YES** — same as user ~D (single call site per storage class) |
| reverse member / base last | **YES** — reuses 17777-17779 ordering |
| local/global/local-static shared | **YES** — all keyed on `cpp_find_dtor_field` |
| N5 lifetime authority | **YES** — no new runtime; extends existing registration |
| duplicate registration | **LOW** — one synthetic field per class |
| link symbol pollution | **SAME AS USER ~D** — mangled, ODR on use |

### DTOR_DESIGN_B — no symbol; scope/global call helpers directly

Concept: extend `cpp_emit_local_dtor` to call subobject helpers when outer has no user ~D`.

**Blocker:** `cpp_emit_member_dtor_calls` / `cpp_emit_base_dtor_calls` require `cpp_this_sym` (4755, 4342).

Possible variant **B'**: temporarily bind `cpp_this_sym` to the stack object in `cpp_emit_local_dtor`, emit subobject dtors, restore. This could work for **LOCAL_AUTO only**.

**Global / local-static** still need fini thunks (`cpp_emit_dtor_thunk`, `cpp_new_local_static_dtor`) that today call **one** dtor global via `cpp_emit_dtor_call_at`. Without a synthetic `~D()` global, each storage class needs a **new ad-hoc wrapper generator** duplicating the member/base walk.

| Criterion | B (pure helper) | B' (temp this, local only) |
|-----------|-----------------|----------------------------|
| local auto | needs temp `cpp_this_sym` | **feasible** |
| global fini | **new thunk type needed** | N/A |
| local static | **new thunk type needed** | N/A |
| symmetry with synthetic ctor | **NO** | **partial** |
| exactly once risk | **higher** (multiple call sites) | medium |

## Recommendation (preflight conclusion)

```text
DTOR_DESIGN_A=RECOMMENDED
DTOR_DESIGN_B=NOT_RECOMMENDED_FOR_SCALAR_SET
DTOR_DESIGN_B_PRIME=LOCAL_AUTO_ONLY_FALLBACK
RECOMMENDED_DTOR_STRATEGY=DTOR_DESIGN_A_SYNTHETIC_DTOR_SYMBOL
```

**Rationale:** All three scalar storage classes already converge on `cpp_find_dtor_field`. Synthetic `~D()` reuses the same registration/emission pipeline as user dtors with **no runtime change**. Design B would fork global/local-static into separate thunk logic.

**Pairing:** Design A synthetic ctor + Design A synthetic dtor is the minimal unified path for N7-02-02.

## Open probe (N7-02-02 production)

Decisive order probe (`n7_02_02_decisive_order.cpp`) must show ctor `B,M1,M2` and dtor `M2,M1,B` with `DOUBLE_CTOR=0` / `DOUBLE_DTOR=0` after implementation.

## STOP check

| # | Condition | Status |
|---|-----------|--------|
| 4 | runtime required for dtor symmetry | **NO** — synthetic ~D uses existing emit |
| 5 | global/local-static separate impl | **NO** with Design A |
| 5 | duplicate destruction | **RISK** — must not emit subobject dtors both inside synthetic ~D AND at decl site; only via ~D() call |

PREFLIGHT_B=PASS (strategy clear; implementation not started)
