# N7-02-01 Source Audit — Implicit Default Constructor Paths

> Read-only audit @ `a098fae`. Evidence: `n7_02_00_measure.bat` + `tccgen.c`.

## 1. User-declared `D() {}` — decisive case trace

```cpp
struct B { B(); };
struct M { M(); };
struct D : B { M m; D() {} };
D d;
```

### Parse / registration

| Step | Function | Lines | Notes |
|------|----------|-------|-------|
| Class body | `struct_decl()` | `11232+` | `is_class==2` for C++ class |
| Ctor name match | `struct_decl()` | `11703-11705` | `v == class_name_tok`, `VT_FUNC` |
| Mem-init `:` | `cpp_save_mem_init_list()` | `11706-11707` | optional |
| Body `{...}` | `skip_or_save_block()` | `11716` | saved to TokenString |
| Register body | `cpp_register_member_body()` | `11718`, `4835-4839` | `field_sym->inline_func_str = body` |
| End of class | `cpp_finish_member_inlines()` | `11768`, `4869-4930` | creates mangled global + `inline_fns` entry |
| `func_ctor` attr | `post_type()` / parse | `9524-9527` | `ad->f.func_ctor = 1` on ctor decl |

### Object declaration `D d;`

| Step | Function | Lines | Notes |
|------|----------|-------|-------|
| FEAT-4F gate | `decl()` | `18102-18200` | local, `cpp_find_ctor_field`, `cpp_class_has_default_ctor` |
| Storage | `decl_initializer_alloc()` | `18125`, `18631` | stack slot |
| Rewrite | token unget | `18183-18187` | `d.D()` |
| Call expr | `expr_eq()` | `18188` | invokes ctor |

### Codegen `D::D()`

| Step | Function | Lines | Notes |
|------|----------|-------|-------|
| Inline replay | `gen_inline_functions()` | `17828-17850` | `begin_macro(body); gen_function(sym)` |
| Prolog + `this` | `gen_function()` | `17563-17642` | `cpp_this_sym` live |
| Implicit bases | `cpp_emit_implicit_base_ctors()` | `17653-17655`, `4302-4326` | calls `B::B(&base)` |
| Implicit members | `cpp_emit_implicit_member_ctors()` | `17659-17660`, `4557-4585` | calls `M::M(&m)` |
| User body | `block(0)` | `17762` | empty `{}` runs here |
| `.init_array` | `gen_function()` | `17602-17603` | if ctor marked (global path) |

### Per-subobject emit helpers

- Base: `cpp_emit_base_default_ctor_call()` @ `4185-4230`
- Member: `cpp_emit_member_default_ctor_call()` @ `4454-4494`

Both require `cpp_this_sym`, resolve 0-arg overload via `cpp_resolve_member_func_call()`.

---

## 2. Implicit `D d;` — fail-closed trace

```cpp
struct B { B(); };
struct M { M(); };
struct D : B { M m; };
D d;
```

| Step | Function | Lines | Outcome |
|------|----------|-------|---------|
| FEAT-4F | `decl()` | `18159` | fails: no `cpp_class_has_default_ctor(D)` |
| Fallback | `decl()` | `18592-18595` | `cpp_validate_decl_default_initialization()` |
| No user ctor | | `4654-4658` | `cpp_validate_implicit_default_ctor(D, 0)` |
| Base walk | | `4622-4624` | `cpp_validate_implicit_default_ctor(B, 2)` |
| Base has `B()` | | `4601-4604` | **error** non-trivial base |
| (or member walk) | | `4633-4634`, `4605-4606` | **error** non-trivial member |

Diagnostic string: `"implicit default construction of non-trivial base is unsupported"` or `"... member ..."`.

**Not reached:** `gen_function`, `cpp_emit_implicit_*`, FEAT-4F rewrite.

---

## 3. Why user outer works but implicit outer fails

`cpp_validate_implicit_default_ctor(class_sym, relation=0)` when `cpp_find_ctor_field(class_sym)` **exists**:

```c
// tccgen.c:4600-4614
if (ctor_field) {
    if (cpp_class_has_default_ctor(class_sym)) {
        if (relation == 2) tcc_error(...);
        if (relation == 1) tcc_error(...);
        return;   /* relation==0: return WITHOUT walking members/bases */
    }
    ...
}
```

User `D() {}` → ctor_field present → validation returns immediately for outer D.

Implicit outer → no ctor_field → full recursive walk → fail on non-trivial viable subobjects.

---

## 4. Design A hook point (proposed, not implemented)

**When:** end of `struct_decl()` for C++ class, after fields known, before or as part of `cpp_finish_member_inlines()` @ `11765-11768`.

**Condition:** `!cpp_find_ctor_field(s)` && class is implicitly default-constructible (new viability-only predicate).

**Action:**

1. `sym_push` ctor field `D` with `VT_FUNC`, empty `inline_func_str` from `{ }` tokens.
2. Let existing `cpp_finish_member_inlines()` register global ctor.

**Use site:** unchanged FEAT-4F/4G + `cpp_class_has_default_ctor()`.

---

## 5. Dtor symmetry gap (audit note)

Local `D d` destruction:

- `cpp_emit_local_dtor()` @ `1565-1597` only emits if `cpp_find_dtor_field(class_sym)`.
- No user `~D()` on implicit class → **no member/base dtor chain at scope exit today**.
- User `~D()` path in `gen_function()` @ `17773-17779` runs member then base dtors.

**N7-02-02 must address:** synthetic `~D()` OR extend scope-exit to call `cpp_emit_member_dtor_calls` / `cpp_emit_base_dtor_calls` for implicitly destructible classes.

---

## 6. N7-01 regression anchor

Negative paths unchanged — still `cpp_validate_decl_default_initialization()` → `tcc_error("class has no default constructor")` for `A(int)`-only classes.

Probe: `n7_02_00_measure.bat` section 11.

---

## 7. STOP conditions — status

| # | Condition | Status |
|---|-----------|--------|
| 1 | Paths not shareable | **CLEAR** — share via Design A |
| 2 | Synthetic breaks overload | **UNKNOWN** — probe in N7-02-02 |
| 3 | No dtor path | **RISK** — synthetic dtor likely needed |
| 4 | Runtime required | **NO** for scalar ctor |
| 5 | global/static separate impl | **NO** — FEAT-4F/4G shared |
| 6 | N5/N6 impact | **NO** if TLS deferred |
| 7 | array inseparable | **DEFERRED** — not blocking scalar |
| 8 | copy/move needed | **NO** |
| 9 | MI/vtable | **NO** for single inheritance case |

**Audit continues to N7-02-02 production — not STOPPED.**
