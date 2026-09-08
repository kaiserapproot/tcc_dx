# N7-02-01 — Implicit Default Constructor Design Freeze

> **Production change: NONE.** Source audit + design comparison only.

## Start authority

```text
=== N7-02-01 START ===
BASE_COMMIT=a098fae
N7_01=COMPLETE
N7_02_00=COMPLETE
USER_DEFAULT_CTOR_CODEGEN_PATH=EXISTING_AUTHORITY
IMPLICIT_DEFAULT_CTOR_SYNTHESIS=MISSING
PRODUCTION_CHANGE=NONE
N7_02_01=IN_PROGRESS
N7_02_IMPLEMENTATION_START=NO
```

## Executive summary

User-declared `D() {}` on a class with non-trivial base/member subobjects already flows through **`gen_function()` → `cpp_emit_implicit_base_ctors()` → `cpp_emit_implicit_member_ctors()` → user body**, with destructor symmetry in **`cpp_emit_member_dtor_calls()` / `cpp_emit_base_dtor_calls()`** at the end of a user `~D()`.

Implicit outer `D d;` (no user `D()`) stops at **`cpp_validate_implicit_default_ctor(..., relation=0)`** inside **`cpp_validate_decl_default_initialization()`** with *"implicit default construction of non-trivial base/member is unsupported"* — **before any synthesis or codegen**. The gap is **missing implicit ctor symbol + missing early-return in validation for viable subobjects**, not missing base/member emit helpers.

**Recommended:** **Design A — synthetic constructor symbol** with empty body `{}`, registered through existing **`cpp_finish_member_inlines()` → `gen_inline_functions()` → `gen_function()`** path so FEAT-4F/4G `obj.Class()` rewrite can reuse the same machinery.

**Not recommended:** Design B (decl-site direct emit).

**Design C** viable only as a thin wrapper around the same emit helpers; still needs ctor invocation wiring and duplicates FEAT-4F.

---

## User default ctor authority chain

| Stage | Authority | Location |
|-------|-----------|----------|
| Parse ctor decl | `struct_decl()` ctor name match + `post_type()` | `tccgen.c:11703-11708`, `9524-9527` |
| Mem-init list save | `cpp_save_mem_init_list()` | `4815-4831` |
| Body save | `skip_or_save_block()` → `cpp_register_member_body()` | `11710-11718`, `4835-4840` |
| Symbol → global ctor | `cpp_finish_member_inlines()` | `4869-4930` |
| Deferred codegen | `gen_inline_functions()` → `gen_function()` | `17828-17857`, `17538+` |
| Base init (implicit) | `cpp_emit_implicit_base_ctors()` | `4302-4326` |
| Member init (implicit) | `cpp_emit_implicit_member_ctors()` | `4557-4585` |
| Mem-init expansion | `gen_function()` mem-init loop | `17667-17759` |
| User body | `block(0)` in `gen_function()` | `17762` |
| Dtor binding (user `~D`) | `cpp_emit_member_dtor_calls()` then `cpp_emit_base_dtor_calls()` | `17773-17779`, `4747-4780`, `4334-4367` |
| Local scope dtor | `cpp_emit_local_dtor()` via scope-exit / `CppLocalInfo` | `1565-1597`, `2019+` |
| Global dyn ctor | FEAT-4G + `cpp_register_global_dyn()` | `decl()` ~18039-18090 |

```text
USER_DEFAULT_CTOR_PARSE_AUTHORITY=struct_decl()+post_type(func_ctor)
USER_DEFAULT_CTOR_SYMBOL_AUTHORITY=cpp_finish_member_inlines()
USER_DEFAULT_CTOR_BASE_INIT_AUTHORITY=cpp_emit_implicit_base_ctors()
USER_DEFAULT_CTOR_MEMBER_INIT_AUTHORITY=cpp_emit_implicit_member_ctors()
USER_DEFAULT_CTOR_BODY_CODEGEN_AUTHORITY=gen_function() block(0)
USER_DEFAULT_CTOR_DTOR_BINDING_AUTHORITY=cpp_emit_member_dtor_calls()+cpp_emit_base_dtor_calls() [user ~D only today]
```

---

## Implicit case — where it stops

Decisive case (`D : B { M m; }; D d;`):

| Stage | Function | Line | Result |
|-------|----------|------|--------|
| Request | `decl()` fallback | `18592-18595` | `cpp_validate_decl_default_initialization()` |
| No user ctor on D | `cpp_find_ctor_field(D)==NULL` | `4654-4658` | enter `cpp_validate_implicit_default_ctor(D,0)` |
| Walk base B | relation=2, B has `B()` | `4601-4604` | **FAIL** "implicit default construction of non-trivial base is unsupported" |
| Walk member M | relation=1, M has `M()` | `4601-4606` | **FAIL** "implicit default construction of non-trivial member is unsupported" |

User outer `D() {}` **skips** the member/base walk at relation=0 when `cpp_find_ctor_field(D)` exists (`4601-4614` early `return`).

```text
IMPLICIT_DEFAULT_CTOR_REQUEST_SITE=decl() -> cpp_validate_decl_default_initialization()
IMPLICIT_DEFAULT_CTOR_VALIDATION_SITE=cpp_validate_implicit_default_ctor()
IMPLICIT_DEFAULT_CTOR_FAIL_CLOSED_SITE=cpp_validate_implicit_default_ctor() lines 4602-4606 (relation 1/2)
IMPLICIT_DEFAULT_CTOR_SYNTHESIS_SITE_CURRENTLY=NONE
```

**Answer:** No ctor symbol is synthesized. Validation rejects **before** `gen_function()` because outer class has **no** `cpp_find_ctor_field` entry; subobject viability is checked with rules that reject *any* user-declared ctor on base/member even when zero-arg viable.

---

## Design comparison

### Design A — synthetic constructor symbol (RECOMMENDED)

After `struct_decl()` completes layout, if class has **no user ctor** and **`CAN_IMPLICIT_DEFAULT_CTOR_EXIST`** (viability-only walk, distinct from N7-01 `cpp_validate_decl_default_initialization` reject path):

1. Inject member-field `D()` with empty body token string `{ }`.
2. Run through existing `cpp_finish_member_inlines()` registration.
3. At use site, existing FEAT-4F/4G sees `cpp_class_has_default_ctor(D)==true` → `d.D()` rewrite → `gen_function()` emit path.

| Criterion | Assessment |
|-----------|------------|
| Reuse base/member emission | **YES** — unchanged `cpp_emit_implicit_*` in `gen_function()` |
| Reuse dtor/lifetime | **PARTIAL** — ctor side yes; local dtor still needs user `~D()` or **synthetic dtor** for member/base chain at scope exit |
| Overload leak | **LOW** if synthetic field is class-member ctor sym only; global mangled via `cpp_ctor_name_tok` like user ctor |
| ODR/link | **Same as inline member ctor** — emitted on first ODR use via `inline_fns` |
| global/local/static sharing | **YES** — same global ctor sym FEAT-4F/4G already use |

```text
DESIGN_A_SYNTHETIC_CTOR_SYMBOL=RECOMMENDED
```

### Design B — decl() direct base/member emit

Emit `B::B()`, `M::M()` from `decl()` without ctor function.

```text
DESIGN_B_DECL_DIRECT_EMIT=NOT_RECOMMENDED
DESIGN_B_RECOMMENDED=NO
```

Reasons: duplicates ctor semantics across storage classes; breaks dtor symmetry; diverges from FEAT-4F/4G; array/TLS paths multiply.

### Design C — synthetic body helper only

`emit_implicit_default_ctor(class_sym, this)` calling `cpp_emit_implicit_base_ctors` + `cpp_emit_implicit_member_ctors` without a symbol.

Still requires **call site** wiring (FEAT-4F-like) and separate dtor path. Becomes Design A minus symbol with **more** ad-hoc call sites.

```text
DESIGN_C_SYNTHETIC_BODY_ONLY=POSSIBLE_BUT_INFERIOR_TO_A
```

```text
RECOMMENDED_DESIGN=DESIGN_A_SYNTHETIC_CTOR_SYMBOL
```

---

## Synthetic ctor visibility (Design A)

| Property | Current evidence | Status |
|----------|------------------|--------|
| External link symbol | User ctors use mangled `cpp_ctor_name_tok` global via `external_sym` | **Same pattern** |
| Emitted only if ODR-used | `gen_inline_functions()` emits when sym referenced | **LIKELY YES** |
| Visible to overload lookup | `cpp_find_ctor_field` / `cpp_class_has_default_ctor` scan member chain | **YES — intentional for FEAT-4F** |
| Visible to user `&D::D` | Would resolve like user ctor global | **UNKNOWN — needs N7-02-02 probe** |
| Implicit-only marker | No field today | **NEEDS `Sym` flag or anon synthetic name policy** |

```text
IMPLICIT_CTOR_HAS_EXTERNAL_SYMBOL=LIKE_USER_CTOR_WHEN_ODR_USED
IMPLICIT_CTOR_EMITTED_ONLY_IF_ODR_USED=LIKELY_YES
IMPLICIT_CTOR_VISIBLE_TO_OVERLOAD_LOOKUP=YES_VIA_cpp_find_ctor_field
IMPLICIT_CTOR_VISIBLE_TO_USER_SYMBOL_LOOKUP=UNKNOWN
IMPLICIT_CTOR_LINKAGE=EXTERNAL_MANGLED_GLOBAL_SAME_AS_USER_CTOR
IMPLICIT_CTOR_EMISSION_POLICY=INLINE_FNS_ON_FIRST_CALL
```

---

## Construction / destruction order (existing — do not reimplement)

**Construction** (`gen_function()` user ctor, `17653-17660`):

1. `cpp_emit_implicit_base_ctors()` — forward member-chain order for bases not in mem-init list
2. `cpp_emit_implicit_member_ctors()` — forward declaration order for members not in mem-init list
3. Explicit mem-init list expansion (`17667+`)
4. User body `block(0)`

Note: comment at `4550-4556` documents accepted deviation — bases all before members (not strict C++ interleaving).

**Destruction** (user `~D()` only, `17773-17779`):

1. User dtor body (already ran before this block in `block(0)`)
2. `cpp_emit_member_dtor_calls()` — reverse decl order (recurse tail-first)
3. `cpp_emit_base_dtor_calls()` — reverse decl order

```text
BASE_INIT_ORDER_AUTHORITY=cpp_emit_implicit_base_ctors()
MEMBER_DECL_ORDER_AUTHORITY=cpp_emit_implicit_member_ctors() forward chain walk
DTOR_REVERSE_MEMBER_ORDER_AUTHORITY=cpp_emit_member_dtor_calls() tail-recurse
BASE_DTOR_LAST_AUTHORITY=cpp_emit_base_dtor_calls() after members
```

---

## Validation vs synthesis separation

| Layer | Function | Role |
|-------|----------|------|
| N7-01 viability (decl site) | `cpp_validate_decl_default_initialization()` | reject impossible default-init |
| Subobject viability | `cpp_class_has_default_ctor()` | zero-arg viable |
| Ill-formed implicit | `cpp_validate_implicit_default_ctor(relation)` | **currently also blocks viable non-trivial subobjects for implicit outer** |
| Codegen | `gen_function()` + `cpp_emit_implicit_*` | **exists for user ctors only** |

N7-02 production must split:

```text
CAN_IMPLICIT_DEFAULT_CTOR_EXIST  -> validation refactor (relation 0/1/2)
HOW_TO_CODEGEN_IMPLICIT_DEFAULT_CTOR -> Design A synthesis + FEAT-4F
```

---

## Runtime / component impact (audit conclusion)

| Component | Required for scalar implicit default ctor? |
|-----------|---------------------------------------------|
| Frontend (`tccgen.c` parse/validate/synth) | **YES** |
| Codegen (`gen_function`, existing emit helpers) | **YES** (reuse) |
| `tccrun` | **NO** (no evidence) |
| `tccelf.c` | **NO** for scalar local/global/static (FEAT-4G `.init_array` already exists) |
| TLS | **DEFERRED** — N6 contract; separate path |

```text
FRONTEND_CHANGE_REQUIRED=YES
CODEGEN_CHANGE_REQUIRED=YES
RUNTIME_CHANGE_REQUIRED=NO
TCCELF_CHANGE_REQUIRED=NO
TCCRUN_CHANGE_REQUIRED=NO
```

---

## Deferred scopes

```text
N7_02_ARRAY_SCOPE=DEFERRED
THREAD_LOCAL_IMPLICIT_DEFAULT_CTOR=DEFERRED_UNLESS_EXISTING_N6_PATH_REUSES_SYNTHESIS
```

N7-02-02 recommended production scope:

```text
N7_02_PRODUCTION_RECOMMENDED_SCOPE=
  scalar implicit default ctor synthesis (Design A)
  storage: local auto, global, local static
  support: base/member default ctor propagation, declaration-order ctor,
           reverse-order dtor (requires synthetic dtor OR scope-exit helper)
  exclude: array, TLS expansion, MI edge cases, overload ranking
```

---

## Open risks (not STOP — track in N7-02-02)

1. **Synthetic dtor** may be required for `CTOR_DTOR_SYMMETRY` on locals when outer has no user `~D()` — `cpp_emit_local_dtor()` only calls user-declared dtor today (`1579-1582`).
2. **`cpp_validate_implicit_default_ctor` relation 1/2** must be relaxed for *viable* subobjects when outer gains synthetic ctor.
3. **Overload / address-of** behavior of synthetic ctor — probe before closing N7-02 production.

---

## Final authority

```text
=== N7-02-01 IMPLICIT DEFAULT CTOR DESIGN FREEZE ===

BASE_COMMIT=a098fae
N7_02_01_COMMIT=<set at closure commit>

RECOMMENDED_DESIGN=DESIGN_A_SYNTHETIC_CTOR_SYMBOL
DESIGN_B_RECOMMENDED=NO

ROOT_CAUSE=IMPLICIT_DEFAULT_CTOR_SYNTHESIS_OR_PROPAGATION
BUG_SCOPE=IMPLICIT_OUTER_DEFAULT_CTOR_MEMBER_AND_BASE_PROPAGATION

N7_02_IMPLEMENTATION_REQUIRED=YES
N7_02_PRODUCTION_RECOMMENDED_SCOPE=scalar local/global/local-static Design A

PRODUCTION_CHANGE=NONE
PUBLIC_API_CHANGE=NONE
RUNTIME_CHANGE=NONE

N7_02_01=PASS
N7_02_IMPLEMENTATION_START=NO
```

Detailed line-level audit: `dev/test/a9/manual/n7_02_01_source_audit.md`

Evidence probes: `dev/test/a9/manual/n7_02_00_*.cpp`, `n7_02_00_measure.bat`
