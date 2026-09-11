=== TCC N7-07C-F1 CAUSAL ISOLATION ===

BASE_HEAD=084f4c1
PHASE=N7-07C-F1
TCC_PRODUCTION_CHANGE=NONE
MEASUREMENT_ONLY=YES

PLAIN_DEFAULT_ARG_SCALAR=PASS
PLAIN_DEFAULT_ARG_ARRAY=COMPILE_FAIL_before_F2
EXTERN_C_DEFAULT_ARG_SCALAR=PASS
EXTERN_C_DEFAULT_ARG_ARRAY=COMPILE_FAIL_before_F2
EXTERN_C_EXPLICIT_ARG_SCALAR=PASS

VEC2_PLAIN_SCALAR=PASS
VEC2_PLAIN_ARRAY=PASS
VEC2_EXTERN_C_SCALAR=PASS
VEC2_EXTERN_C_ARRAY=COMPILE_FAIL_before_F2

ARRAY_FAIL_DIAGNOSTIC=implicit default construction via default arguments is unsupported
SCALAR_EXTERN_C_LINKAGE=NOT_REQUIRED_FOR_PASS

ROOT_SCOPE=LOCAL_ARRAY_CTOR_EMITTER_DEFAULT_ARGUMENT_HANDLING
ROOT_SCOPE_NOTE=scalar uses FEAT-4F expr_eq path with cpp_apply_default_args; array uses cpp_emit_class_default_ctor_call guard at cpp_func_param_count!=0

EXTERN_C_ARRAY_NOTE=cpp_resolve_member_func_call returns NULL when extern_c; cpp_resolve_func_call falls back to sym_find without default-arg viability, so vec2() 0-arg ctor mis-resolves to vec2(float,float) in array emitter only

DIRECT_CAUSE=ctor zero-arg viability succeeds on declaration side but cpp_emit_class_default_ctor_call rejects nonzero declared param count without materializing defaults
ROOT_CAUSE=implicit default-construction emission does not call cpp_apply_default_args after overload resolution
ROOT_CAUSE_CONFIRMED=YES
