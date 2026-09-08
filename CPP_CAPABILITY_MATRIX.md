# CPP Capability Matrix (Post-N6-00)

Status: **POST-N6-00 INVENTORY** (doc/test only; no production change).

Authority gate: `dev/test/a9/manual/post_n6_00_inventory.bat`

```text
=== POST-N6-00 START ===
BASE_COMMIT=3747073
N6=COMPLETE
N6_THREAD_LOCAL=COMPLETE
PRODUCTION_CHANGE=NONE
PUBLIC_API_CHANGE=NONE
LANGUAGE_FEATURE_CHANGE=NONE
POST_N6_00=IN_PROGRESS
N7_START=NO
```

---

## 1. Classification legend

| Label | Meaning |
|-------|---------|
| **SUPPORTED** | Parse + semantics + codegen + runtime behave as C++98 subset expects for supported shapes |
| **LIMITED** | Supported subset with documented execution-mode or semantic gaps |
| **FAIL_CLOSED** | Rejected at compile time (or runtime abort where specified) with diagnostic; no bad code emission |
| **UNSUPPORTED** | Not implemented; may fail at parse or later |
| **UNKNOWN** | Compiles or partially wired; runtime / all execution modes not proven |

Evidence levels:

| Level | Meaning |
|-------|---------|
| **TESTED** | `dev/test` gate or manual measure with exit-code / marker authority |
| **SOURCE_AUDITED** | Code path reviewed; no dedicated runtime gate |
| **DOC_ONLY** | Documented limitation only |
| **UNKNOWN** | Not verified in this inventory |

Silent-acceptance taxonomy (N7 selection):

| Label | Meaning |
|-------|---------|
| **SUPPORTED_AND_CORRECT** | Works as documented |
| **SUPPORTED_WITH_LIMITATIONS** | Works within stated limits |
| **FAIL_CLOSED** | Rejects invalid uses |
| **UNSUPPORTED_SILENT_ACCEPTANCE** | Compiles without required diagnostic; may miscompile or UB |

---

## 2. Execution modes

| Mode | Scope in this matrix |
|------|----------------------|
| **NORMAL_EXE** | `tcc.exe` linked EXE, PE TLS callbacks active |
| **`-run` / tcc_run()** | In-process relocated image; N6 worker wrappers |
| **libtcc direct relocate** | Host calls manual `main()`; LIMITED cleanup API |
| **DLL** | C++ TU in `-shared` / DLL load (mostly FAIL_CLOSED for N6 TLS; other features largely UNVERIFIED) |

N6 thread_local authority (do not re-test here): `n6_08_final_regression.bat` @ master `3747073`.

---

## 3. Master capability matrix

Columns: **Parse / Codegen / Link / Runtime** = PASS | FAIL | PARTIAL | N/A  
**Class** = primary inventory classification. **Evidence** = proof level.

| # | Feature area | Parse | Codegen | Link | Runtime | NORMAL_EXE | `-run` | libtcc | DLL | Class | Evidence |
|---|--------------|-------|---------|------|---------|------------|--------|--------|-----|-------|----------|
| 1 | **Constructors / destructors** (user-declared, mem-init, OOC) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat4c_*`, `feat4e_*`, `feat4g_*`) |
| 1b | Implicit default ctor `P f;` when no viable default ctor | PASS | PARTIAL | PASS | FAIL | LIMITED | LIMITED | UNKNOWN | UNKNOWN | **UNSUPPORTED_SILENT_ACCEPTANCE** | TESTED (`post_n6_00_silent_no_default_ctor.cpp`) |
| 1c | Implicit member/base ctor/dtor synthesis | PARTIAL | PARTIAL | PASS | PARTIAL | LIMITED | LIMITED | UNKNOWN | UNKNOWN | FAIL_CLOSED | TESTED (`a9/negative/implicit_*`, `default_*`) |
| 2 | **Global / static storage objects** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat4g_*`, N6-05 order gates) |
| 3 | **Function-local static** (ctor/dtor) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED_WITH_LIMITATION | UNKNOWN | FAIL_CLOSED | LIMITED | TESTED (`pr_n5_local_static_dtor.bat`, N5) |
| 4 | **`thread_local`** (N6 subset) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | LIMITED | FAIL_CLOSED | SUPPORTED / LIMITED | TESTED (N6-08 authority) |
| 5 | **Single inheritance** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`inherit_*`, `feat4d_*`) |
| 6 | **Virtual functions / vtable** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat5a_*`, G5/G6) |
| 7 | **Multiple inheritance** (non-virtual) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | LIMITED | TESTED (`mi_*`, `vmi_*`; diamond / deep secondary limits) |
| 7b | Virtual inheritance / diamond | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | TESTED (`post_n6_00_probe_virtual_inheritance.cpp`, `g6_deep_secondary`) |
| 8 | **Copy constructor** (explicit + implicit paths) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`copy_init_*`, BUG-46/47) |
| 9 | **Copy assignment** | PARTIAL | PARTIAL | PASS | PARTIAL | FAIL_CLOSED (implicit) / SUPPORTED (user `operator=`) | same | UNKNOWN | UNKNOWN | FAIL_CLOSED + SUPPORTED | TESTED (negative Phase 3 + `feat6a_ext2_*`) |
| 10 | **Move ctor / move assignment** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | TESTED (`post_n6_00_probe_move_semantics.cpp`) |
| 11 | **References** (lvalue binding, locals, params) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat6a_ref_arg`, BUG-9) |
| 12 | **Operator overloading** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat6a_*`, G-OP, ext5/6) |
| 13 | **`new` / `delete`** (scalar class) | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`g4_*`, G6 virtual delete) |
| 13b | `new T[n]` / `delete[]` for class types | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | TESTED (`g4_new_array_class`) |
| 14 | **Class object arrays** (members, locals) | PARTIAL | PARTIAL | PASS | PARTIAL | FAIL_CLOSED (non-POD dtor arrays) | same | UNKNOWN | UNKNOWN | FAIL_CLOSED | TESTED (`implicit_class_array_member`, `return_*_array_*`) |
| 15 | **Temporary objects / lifetime** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`pr_n3a_temp_path.bat`) |
| 16 | **RAII / scope destruction** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`feat4e_*`, `return_scope_dtor.cpp`) |
| 16b | Scope exit via `goto` / switch / loop | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`pr_n4_goto_lifetime.bat`, `pr_n4_break_continue_dtor.bat`) |
| 17 | **Exceptions** (`try`/`catch`/`throw`) | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | TESTED (`post_n6_00_probe_exceptions.cpp`) |
| 18 | **RTTI** (`dynamic_cast`, `typeid`) | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | TESTED (`post_n6_00_probe_rtti.cpp`) |
| 19 | **Templates** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | TESTED (`post_n6_00_probe_template.cpp`) |
| 20 | **Function overloading** | PASS | PASS | PASS | PASS | LIMITED | LIMITED | UNKNOWN | UNKNOWN | LIMITED | TESTED (`govl_*`; ISO rank/ambiguous not implemented) |
| 21 | **Namespaces** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | TESTED (`post_n6_00_probe_namespace.cpp`) |
| 22 | **`enum class`** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | SOURCE_AUDITED (parse probe) |
| 23 | **Lambda expressions** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | TESTED (`post_n6_00_probe_lambda.cpp`) |
| 24 | **`constexpr`** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | TESTED (`post_n6_00_probe_constexpr.cpp`) |
| 25 | **Default member initialization** | FAIL | — | — | — | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | UNSUPPORTED | SOURCE_AUDITED (in-class `=`) |
| 26 | **Delegating constructors** | FAIL | — | — | — | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | DOC_ONLY |
| 27 | **`initializer_list`** | FAIL | — | — | — | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | UNSUPPORTED | DOC_ONLY |
| 28 | **Placement `new`** | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | UNKNOWN | DOC_ONLY |
| 29 | **Static data members** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`static_member.cpp`, BUG-33 link) |
| 30 | **Inline functions / ODR** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`inline_member.cpp`, G7 link) |
| 31 | **C / C++ mixed TU** | PASS | PASS | PASS | PASS | SUPPORTED | SUPPORTED | UNKNOWN | UNKNOWN | SUPPORTED | TESTED (`mixed_link.bat`, `a2/*`) |
| 32 | **Execution-mode matrix** (cross-cutting) | — | — | — | — | SUPPORTED | SUPPORTED | LIMITED | FAIL_CLOSED | LIMITED | TESTED (N6-08 + N6-07 + H0-00) |

### 3.1 thread_local detail (authority reuse)

| Form | NORMAL_EXE | `-run` | libtcc relocate | DLL | Class | Evidence |
|------|------------|--------|-----------------|-----|-------|----------|
| trivial / user ctor+dctor | SUPPORTED | SUPPORTED | LIMITED | FAIL_CLOSED | SUPPORTED | N6-08 TESTED |
| 15 unsupported TLS shapes | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | FAIL_CLOSED | N6-07-05 inventory |
| silent acceptance count | 0 | 0 | 0 | 0 | — | N6-08 authority |

---

## 4. Silent acceptance register

| ID | Feature | Symptom | Classification | Probe / gate |
|----|---------|---------|----------------|--------------|
| SA-01 | Local `P f;` without default ctor | Compiles; storage uninitialized; no diagnostic | **UNSUPPORTED_SILENT_ACCEPTANCE** | `post_n6_00_silent_no_default_ctor.cpp` |
| — | Implicit copy assignment (unsafe types) | Was silent → now compile error | FAIL_CLOSED (fixed) | `run_all.bat` Phase 3, 57 negatives |
| — | Value slicing polymorphic class | Was silent → now compile error | FAIL_CLOSED (fixed) | `copy_init_slicing`, `bug49_slice_by_value` |
| — | `new T[n]` class arrays | Compile error | FAIL_CLOSED | `g4_new_array_class` |

**SILENT_ACCEPTANCE_COUNT=1** (SA-01 only in Post-N6-00 probe set)  
**SILENT_MISCOMPILE_COUNT=1** (same; runtime UB / wrong values)

---

## 5. N7 candidate ranking (SAFETY > FOUNDATIONAL > USER_VALUE)

| Rank | Candidate | A.SAFETY | B.FOUNDATIONAL | C.USER_VALUE | Notes |
|------|-----------|----------|----------------|--------------|-------|
| 1 | **Implicit default construction / no viable default ctor** (`P f;` when only `P(int)`) | HIGH | HIGH | HIGH | Only confirmed silent miscompile in Post-N6-00 probes; standard fix is COMPILE_FAIL not implicit default ctor synthesis |
| 2 | **Implicit special-member completion** (align negative inventory with all decl forms) | HIGH | HIGH | MED | Many shapes already FAIL_CLOSED; gap is default-construction declaration |
| 3 | **Overload conversion ranking / ambiguity** | MED | HIGH | MED | Two-level score; chain-first-wins; not ISO |
| 4 | MI vtable edge cases (deep secondary, overloaded virtual) | MED | MED | LOW | Partial FAIL_CLOSED exists (`g6_deep_secondary`, `vmi_overloaded_virtual`) |
| 5 | Exceptions / RTTI / templates | LOW (fail-closed today) | MED | HIGH | Large scope; not silent today |

**RECOMMENDED_N7=CLASS_DEFAULT_INITIALIZATION**

N7 roadmap (spec only; `N7_START=NO` until N7-00 freeze):

| Phase | Scope |
|-------|--------|
| N7-00 | Default-initialization capability / semantics freeze (production none) |
| N7-01 | No-viable-default-constructor fail-closed (silent miscompile 1 → 0) |
| N7-02 | Implicit default ctor codegen (trivial / base / member propagation) |
| N7-03 | Array default-construction propagation |
| N7-04 | Storage-class regression (local / global / static / TLS) |
| N7-05 | Implicit-special-member interaction audit |
| N7-06 | Negative / fail-closed qualification |
| N7-07 | Full regression / closure |

---

## 6. Inventory counts (feature-area rollup)

**Counting semantics (non-exclusive):**

```text
CAPABILITY_COUNTS_ARE_NON_EXCLUSIVE=YES
COUNTING_UNIT=FEATURE_X_STAGE_OR_EXECUTION_MODE
FEATURE_AREA_COUNT=32
```

`FEATURE_COUNT=32` is the number of **top-level feature areas** in section 3 (rows `#1`–`#32`).
`SUPPORTED_COUNT`, `LIMITED_COUNT`, `FAIL_CLOSED_COUNT`, `UNSUPPORTED_COUNT`, and `UNKNOWN_COUNT` are **non-exclusive tags** applied across matrix rows, sub-rows (e.g. `#1b`, `#7b`, `#13b`), execution-mode cells, and stage columns. A single sub-row may contribute to more than one bucket (e.g. thread_local = SUPPORTED + FAIL_CLOSED for unsupported forms). **These counts do not sum to 32.**

| Metric | Count | Notes |
|--------|-------|-------|
| FEATURE_COUNT / FEATURE_AREA_COUNT | 32 | Top-level areas in section 3 (`#1`–`#32`) |
| SUPPORTED_COUNT | 18 | Tag hits: primary class SUPPORTED or SUPPORTED/LIMITED with working core |
| LIMITED_COUNT | 9 | Tag hits: execution-mode or semantic subsets |
| FAIL_CLOSED_COUNT | 28 | Tag hits: sub-rows, N6 TLS unsupported forms, negative corpus |
| UNSUPPORTED_COUNT | 14 | Tag hits: C++11+ and unimplemented ISO features |
| UNKNOWN_COUNT | 6 | Tag hits: mostly libtcc/DLL paths for otherwise-supported features |
| SILENT_ACCEPTANCE_COUNT | 1 | SA-01 only (inventory probe set) |
| SILENT_MISCOMPILE_COUNT | 1 | SA-01 |

Inventory gate integration:

```text
POST_N6_INVENTORY_STANDALONE=YES
RUN_ALL_INTEGRATION=DEFERRED
```

Rationale: `post_n6_00_inventory.bat` currently **documents** SA-01 as PASS; adding it to permanent `run_all` would freeze “1 silent miscompile exists” as normal. Integrate after N7-01 inverts SA-01 to compile-fail.

---

## 7. Existing gate index (reuse; do not duplicate)

| Domain | Authority batch / gate |
|--------|-------------------------|
| N6 thread_local | `n6_08_final_regression.bat` |
| N5 local static dtor | `pr_n5_local_static_dtor.bat` |
| N3A temporary lifetime | `pr_n3a_temp_path.bat` |
| N4 goto / loop scope dtor | `pr_n4_goto_lifetime.bat`, `pr_n4_break_continue_dtor.bat` |
| Negative / fail-closed corpus | `run_all.bat` Phase 3 (57 `.cpp`) |
| CPPUnit G7 | `sample/cppunit/build_cppunit.bat` |
| Full tree | `build.bat` @ master |

---

## 8. POST-N6-00 closure authority

```text
=== POST-N6-00 FINAL ===
BASE_COMMIT=3747073
POST_N6_00_COMMIT=065e26d
POST_N6_00_MERGE_COMMIT=7dfe16e
POST_N6_00_CLOSURE_COMMIT=b229b4c

FEATURE_COUNT=32
FEATURE_AREA_COUNT=32
CAPABILITY_COUNTS_ARE_NON_EXCLUSIVE=YES
COUNTING_UNIT=FEATURE_X_STAGE_OR_EXECUTION_MODE
SUPPORTED_COUNT=18
LIMITED_COUNT=9
FAIL_CLOSED_COUNT=28
UNSUPPORTED_COUNT=14
UNKNOWN_COUNT=6
SILENT_ACCEPTANCE_COUNT=1
SILENT_MISCOMPILE_COUNT=1
HIGH_RISK_FEATURES=LOCAL_AUTO_NO_DEFAULT_CTOR
FOUNDATIONAL_GAPS=IMPLICIT_SPECIAL_MEMBERS,OVERLOAD_RANKING,MI_VTABLE_EDGES
N7_CANDIDATE_1=CLASS_DEFAULT_INITIALIZATION
N7_CANDIDATE_1_REASON=SAFETY:no_viable_default_ctor_must_compile_fail_not_uninitialized_object
N7_CANDIDATE_2=IMPLICIT_SPECIAL_MEMBER_COMPLETION
N7_CANDIDATE_2_REASON=SAFETY+FOUNDATIONAL:member_and_base_no_default_ctor_propagation
N7_CANDIDATE_3=OVERLOAD_CONVERSION_RANKING
N7_CANDIDATE_3_REASON=FOUNDATIONAL:two_level_scoring_and_chain_first_wins_not_ISO
RECOMMENDED_N7=CLASS_DEFAULT_INITIALIZATION
POST_N6_INVENTORY_STANDALONE=YES
RUN_ALL_INTEGRATION=DEFERRED
PRODUCTION_CHANGE=NONE
PUBLIC_API_CHANGE=NONE
POST_N6_00_INVENTORY_GATE=PASS
POST_N6_00=COMPLETE
N7_START=NO
```

Gate: `dev/test/a9/manual/post_n6_00_inventory.bat` must exit 0 (standalone; not in `run_all.bat`).
