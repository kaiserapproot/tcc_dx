#!/bin/sh
# run_all_mac.sh - macOS port of run_all.bat (the platform-neutral phases).
#
#   TCC_EXE=/path/to/tcc sh dev/test/run_all_mac.sh
#
# Default compiler: build/macos/tcc (made by build_mac.sh).
# Phases that are kept: 1 compile-only, 2 link + run (exit 0), 2b -run,
# 3 negative, the multi-TU link gates, 4 crash corpus, 5 CPPUnit (G7),
# plus the macOS-only regressions in dev/test/macos/
# Windows-only gates are skipped: they test dev\include (Win32 SDK, intrin,
# DirectX GUIDs), gdb/DWARF on PE, and the thread_local runtime, which is
# built on Win32 TLS (TlsAlloc / CreateThread hooks) and is not ported.
# Sources that cannot work on macOS are listed in MAC_SKIP with a reason.

cd "$(dirname "$0")" || exit 1
HERE=$(pwd)
ROOT=$(cd ../.. && pwd)
TCC=${TCC_EXE:-$ROOT/build/macos/tcc}
case "$TCC" in /*) ;; *) TCC="$ROOT/$TCC" ;; esac
if [ ! -x "$TCC" ]; then
    echo "run_all_mac.sh: compiler not found: $TCC (run ./build_mac.sh first)" >&2
    exit 1
fi

FAILED=0
CRASHES=0
WORK=$(mktemp -d "${TMPDIR:-/tmp}/tcc_run_all_mac.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
LOG="$WORK/err.log"

# Same list as run_all.bat: documented gaps, compile failure is non-gating.
KNOWNFAIL=" sm3.cpp sm6.cpp t_assign.cpp "
# macOS: sources that need the Win32 SDK or the Win32 thread_local runtime.
MAC_SKIP=" $(cd "$HERE" && grep -l -E '#include *<(windows|winbase|process|conio|intrin|d3d[0-9a-z]*|objbase|guiddef|malloc)\.h>|thread_local|__declspec|__stdcall' a9/*.cpp a9/*.c 2>/dev/null | tr '\n' ' ') "

in_list() { case "$2" in *" $1 "*) return 0 ;; esac; return 1; }

# exit status of a signal death is 128+N in sh; tcc's own failure is 1.
# A crash is either a signal (>=128) or the crash net's marker on stderr.
is_crash() { [ "$1" -ge 128 ] || grep -q "internal error" "$2" 2>/dev/null; }

# === Phase 1: compile-only (every source must pass -c) ===
for f in smoke/*.c smoke/*.cpp a2/*.c a2/*.cpp a3/*.c a3/*.cpp a4/*.cpp \
         a5/*.cpp a6/*.cpp a8/*.cpp a9/*.c a9/*.cpp a7/member_call.cpp \
         a7/default_arg.cpp a7/inline_member.cpp a7/typedef_class.cpp; do
    [ -f "$f" ] || continue
    if in_list "$f" "$MAC_SKIP"; then
        echo "=== $f === [skipped on macOS]"
        continue
    fi
    echo "=== $f ==="
    "$TCC" -c "$f" -o "$WORK/p1.o" 2>"$LOG"
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  [CRASH] $f exit=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    elif [ $ec -ne 0 ]; then
        cat "$LOG"
        if in_list "$(basename "$f")" "$KNOWNFAIL"; then
            echo "  [known gap, non-gating] $f"
        else
            echo "  [COMPILE FAIL] $f"; FAILED=$((FAILED + 1))
        fi
    fi
done

# === Phase 2: execution gate (link + run, require exit code 0) ===
for f in a9/*.cpp a7/member_call.cpp a7/default_arg.cpp a7/inline_member.cpp \
         a7/typedef_class.cpp smoke/hello.c; do
    [ -f "$f" ] || continue
    in_list "$f" "$MAC_SKIP" && continue
    in_list "$(basename "$f")" "$KNOWNFAIL" && continue
    exe="$WORK/$(basename "${f%.*}")"
    "$TCC" "$f" -o "$exe" >/dev/null 2>"$LOG"
    ec=$?
    if [ $ec -ne 0 ]; then
        cat "$LOG"
        if is_crash $ec "$LOG"; then
            echo "  [CRASH] $f exit=$ec"; CRASHES=$((CRASHES + 1))
        else
            echo "  [BUILD FAIL] $f"
        fi
        FAILED=$((FAILED + 1))
    else
        "$exe" >/dev/null 2>&1
        rc=$?
        if [ $rc -ne 0 ]; then
            echo "  [RUN FAIL] $f exit=$rc"; FAILED=$((FAILED + 1))
        fi
    fi
done

# === Phase 2b: -run gate (BUG-50: second compile -> tccgen_init) ===
echo "=== Phase 2b: -run a9/local_static_dtor.cpp ==="
"$TCC" -run a9/local_static_dtor.cpp >"$LOG" 2>&1
ec=$?
if is_crash $ec "$LOG"; then
    echo "  [CRASH] -run a9/local_static_dtor.cpp exit=$ec"; cat "$LOG"
    CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
elif [ $ec -ne 0 ]; then
    cat "$LOG"; echo "  [RUN FAIL] -run a9/local_static_dtor.cpp"
    FAILED=$((FAILED + 1))
fi

# === Phase 3: negative gate (compilation MUST fail) ===
# .expected holds an ASCII pattern ("<name>.cpp:<line>: error:").
for f in a9/negative/*.cpp; do
    [ -f "$f" ] || continue
    in_list "$f" "$MAC_SKIP" && continue
    echo "=== $f [negative] ==="
    "$TCC" -c "$f" -o "$WORK/neg.o" >"$LOG" 2>&1
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  [CRASH] $f exit=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    elif [ $ec -eq 0 ]; then
        echo "  [NEGATIVE FAIL] $f compiled but was expected to be rejected"
        FAILED=$((FAILED + 1))
    elif [ -f "${f%.cpp}.expected" ]; then
        pat=$(tr -d '\r' < "${f%.cpp}.expected" | tail -n 1)
        # run_all.bat runs inside dev\test with "a9\negative\x.cpp"; tcc
        # prints the path as given, so match on the file-name part only.
        pat=${pat##*[\\/]}
        if ! grep -F -q -- "$pat" "$LOG"; then
            echo "  [NEGATIVE FAIL] $f rejected, but not with \"$pat\""
            cat "$LOG"; FAILED=$((FAILED + 1))
        fi
    fi
done

# === PR-N4 goto lifetime (port of a9/manual/pr_n4_goto_lifetime.bat) ===
# The sources have no #include, so the gate is platform-neutral.  Cases
# 1-13 and 15-18 of pr_n4_goto_lifetime.cpp must exit 0 (18 is
# SYMBOL_REUSE_NO_FALSE_DTOR); the pr_n4_goto_unsafe_*.cpp sources must be
# rejected with the "goto into a scope requiring initialization" diagnostic
# (fail-closed) and must never crash the compiler.
echo "=== a9/manual/pr_n4_goto_lifetime ==="
N4_EXE="$WORK/pr_n4_goto_lifetime"
N4_FAILED=0
"$TCC" a9/manual/pr_n4_goto_lifetime.cpp -o "$N4_EXE" >"$LOG" 2>&1
ec=$?
if [ $ec -ne 0 ]; then
    cat "$LOG"
    if is_crash $ec "$LOG"; then CRASHES=$((CRASHES + 1)); fi
    echo "  PR_N4_GOTO_LIFETIME=COMPILE_FAIL"; FAILED=$((FAILED + 1))
else
    for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 15 16 17 18; do
        "$N4_EXE" "$n" >/dev/null 2>&1
        rc=$?
        if [ $rc -ne 0 ]; then
            echo "  PR_N4 case $n=FAIL exit=$rc"; N4_FAILED=1
        fi
    done
    for f in a9/manual/pr_n4_goto_unsafe.cpp \
             a9/manual/pr_n4_goto_unsafe_scalar.cpp \
             a9/manual/pr_n4_goto_unsafe_reference.cpp \
             a9/manual/pr_n4_goto_unsafe_aggregate.cpp \
             a9/manual/pr_n4_goto_unsafe_lca.cpp; do
        "$TCC" -c "$f" -o "$WORK/n4_unsafe.o" >"$LOG" 2>&1
        ec=$?
        if is_crash $ec "$LOG"; then
            echo "  $f=CRASH exit=$ec"; cat "$LOG"
            CRASHES=$((CRASHES + 1)); N4_FAILED=1
        elif [ $ec -eq 0 ]; then
            echo "  $f=FAIL (accepted, expected fail-closed)"; N4_FAILED=1
        elif ! grep -F -q "goto into a scope requiring initialization is unsupported" "$LOG"; then
            echo "  $f=WRONG_DIAGNOSTIC"; cat "$LOG"; N4_FAILED=1
        fi
    done
    if [ $N4_FAILED -ne 0 ]; then
        echo "  PR_N4_GOTO_LIFETIME=FAIL"; FAILED=$((FAILED + 1))
    else
        echo "  PR_N4_GOTO_LIFETIME=PASS"
    fi
fi

# === multi-TU link gates (a2/mixed_link.bat, a9/*_link.bat) ===
build_run_ok() { # tag, sources...
    tag=$1; shift
    if ! "$TCC" "$@" -o "$WORK/$tag" >"$LOG" 2>&1; then
        echo "  $tag=BUILD_FAIL"; cat "$LOG"; FAILED=$((FAILED + 1)); return
    fi
    if ! "$WORK/$tag" >/dev/null 2>&1; then
        echo "  $tag=RUN_FAIL"; FAILED=$((FAILED + 1)); return
    fi
    echo "  $tag=PASS"
}
build_must_fail() { # tag, pattern, sources...
    tag=$1; pat=$2; shift 2
    "$TCC" "$@" -o "$WORK/$tag" >"$LOG" 2>&1
    ec=$?
    if [ $ec -eq 0 ]; then
        echo "  $tag=SILENT_ACCEPTANCE"; FAILED=$((FAILED + 1))
    elif is_crash $ec "$LOG"; then
        echo "  $tag=TCC_CRASH rc=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    elif ! grep -F -q -- "$pat" "$LOG"; then
        echo "  $tag=REJECTED_WITH_WRONG_DIAGNOSTIC expected=$pat"; cat "$LOG"
        FAILED=$((FAILED + 1))
    else
        echo "  $tag=REJECTED_AS_EXPECTED"
    fi
}

# mixed_link.bat only compiles the two TUs (-c foo.cpp bar.c); the objects
# are then linked here and run.  bar.c's main returns its "class" (1).
echo "=== a2/mixed_link ==="
(cd "$WORK" && "$TCC" -c "$HERE/a2/foo.cpp" "$HERE/a2/bar.c") >"$LOG" 2>&1
ec=$?
if is_crash $ec "$LOG"; then
    echo "  MIXED_COMPILE=TCC_CRASH rc=$ec"; cat "$LOG"
    CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
elif [ $ec -ne 0 ]; then
    echo "  MIXED_COMPILE=FAIL"; cat "$LOG"; FAILED=$((FAILED + 1))
else
    echo "  MIXED_COMPILE=PASS"
    "$TCC" "$WORK/foo.o" "$WORK/bar.o" -o "$WORK/mixed" >"$LOG" 2>&1
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  MIXED_LINK=TCC_CRASH rc=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    elif [ $ec -ne 0 ]; then
        echo "  MIXED_LINK=BUILD_FAIL"; cat "$LOG"; FAILED=$((FAILED + 1))
    else
        "$WORK/mixed"
        ec=$?
        if [ $ec -eq 1 ]; then echo "  MIXED_LINK=PASS"
        else echo "  MIXED_LINK=RUN_FAIL exit=$ec (expected 1)"; FAILED=$((FAILED + 1)); fi
    fi
fi

# The Windows-only cases of dcl_link.bat (two TUs including windows.h, GUIDs)
# are left out.  The undefined-symbol message is printed in Japanese by the
# Mach-O linker, so match on the (underscore-prefixed) symbol name only.
echo "=== a9/link gates ==="
cd a9/link || exit 1
build_run_ok GOVL_LINK govl_link_main.cpp govl_link_lib.cpp
build_run_ok BUG33_LINK bug33_link_main.cpp bug33_link_lib.cpp
build_run_ok DCL_LINK_A_SINGLE_FORM_DECLARES dcl_link_a_use.cpp dcl_link_a_def.c
build_must_fail DCL_LINK_A_ALONE_UNDEFINED "'_dcl_a'" dcl_link_a_use.cpp
build_run_ok DCL_LINK_B_BLOCK_FORM_DEFINES dcl_link_b_block.cpp
build_run_ok DCL_LINK_C_CLASS_SINGLE_FORM_DECLARES dcl_link_c_class.cpp
build_run_ok DCL_LINK_D_INITIALIZER_DEFINES dcl_link_d_init.cpp
build_must_fail DCL_LINK_BLOCK_CLASS_STILL_NEEDS_CTOR "class has no default constructor" dcl_link_neg_block_class.cpp
build_run_ok DCL_LINK_E_DEF_KEEPS_C_LINKAGE dcl_link_e_def.cpp dcl_link_e_use.c
build_run_ok DCL_LINK_F_CALL_IN_BLOCK_RESOLVES dcl_link_f_resolve.cpp
cd "$HERE" || exit 1

# === macOS-only regressions (dev/test/macos/*.cpp, exit 0 required) ===
echo "=== macos/ ==="
for f in macos/*.c macos/*.cpp; do
    [ -f "$f" ] || continue
    tag=$(basename "${f%.*}")
    build_run_ok "MACOS_$tag" "$f"
    # -run must agree with the linked executable
    "$TCC" -run "$f" >/dev/null 2>"$LOG"
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  [CRASH] -run $f exit=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    elif [ $ec -ne 0 ]; then
        echo "  [RUN FAIL] -run $f exit=$ec"; cat "$LOG"; FAILED=$((FAILED + 1))
    fi
done
# link/: TUs compiled one by one with -c, then linked; stdout must match
# the .expected file (constructor / destructor order included)
link_expect() { # tag, sources...
    tag=$1; shift
    objs=
    for s in "$@"; do
        o="$WORK/$(basename "${s%.*}").o"
        "$TCC" -c "$s" -o "$o" >"$LOG" 2>&1
        ec=$?
        if is_crash $ec "$LOG"; then
            echo "  $tag=TCC_CRASH rc=$ec compiling $s"; cat "$LOG"
            CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1)); return
        elif [ $ec -ne 0 ]; then
            echo "  $tag=COMPILE_FAIL $s"; cat "$LOG"; FAILED=$((FAILED + 1)); return
        fi
        objs="$objs $o"
    done
    # $objs is split on purpose: WORK comes from mktemp and has no blanks
    "$TCC" $objs -o "$WORK/$tag" >"$LOG" 2>&1
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  $tag=TCC_CRASH rc=$ec linking"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1)); return
    elif [ $ec -ne 0 ]; then
        echo "  $tag=LINK_FAIL"; cat "$LOG"; FAILED=$((FAILED + 1)); return
    fi
    "$WORK/$tag" >"$WORK/$tag.out" 2>&1
    ec=$?
    if [ $ec -ne 0 ]; then
        echo "  $tag=RUN_FAIL exit=$ec"; FAILED=$((FAILED + 1))
    elif ! diff "macos/link/$tag.expected" "$WORK/$tag.out" >"$LOG" 2>&1; then
        echo "  $tag=OUTPUT_MISMATCH"; cat "$LOG"; FAILED=$((FAILED + 1))
    else
        echo "  $tag=PASS"
    fi
}
link_expect static_dtor_link \
    macos/link/static_dtor_link_a.cpp macos/link/static_dtor_link_b.cpp
link_expect implicit_dtor_sret \
    macos/link/implicit_dtor_sret_a.cpp macos/link/implicit_dtor_sret_b.cpp

# interop/: ABI agreement with clang.  The clang++ side goes into a dylib,
# the tcc side (main) links against it, and the program checks both
# directions of the call (stdout "... ok" lines, exit 0).
echo "=== clang interop ==="
CXX=${CXX:-clang++}
interop_expect() { # tag, clang source, tcc source
    tag=$1
    if ! "$CXX" -arch x86_64 -w -dynamiclib "$2" -o "$WORK/lib$tag.dylib" >"$LOG" 2>&1; then
        echo "  $tag=CLANG_BUILD_FAIL"; cat "$LOG"; FAILED=$((FAILED + 1)); return
    fi
    "$TCC" "$3" -L"$WORK" -l"$tag" -o "$WORK/$tag" >"$LOG" 2>&1
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  $tag=TCC_CRASH rc=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1)); return
    elif [ $ec -ne 0 ]; then
        echo "  $tag=BUILD_FAIL"; cat "$LOG"; FAILED=$((FAILED + 1)); return
    fi
    DYLD_LIBRARY_PATH="$WORK" "$WORK/$tag" >"$WORK/$tag.out" 2>&1
    ec=$?
    if [ $ec -ne 0 ]; then
        echo "  $tag=RUN_FAIL exit=$ec"; cat "$WORK/$tag.out"; FAILED=$((FAILED + 1))
    else
        echo "  $tag=PASS"
    fi
}
interop_expect byval macos/interop/byval_clang.cpp macos/interop/byval_tcc.cpp
# negative: the link (not just -c) must fail with the .expected text
for f in macos/negative/*.c macos/negative/*.cpp; do
    [ -f "$f" ] || continue
    build_must_fail "MACOS_NEG_$(basename "${f%.*}")" \
        "$(tr -d '\r' < "${f%.*}.expected" | tail -n 1)" "$f"
done

# === Phase 4: crash corpus (C2) - only the compiler surviving is gated ===
for f in ../../sample/cppunit/*.cpp; do
    [ -f "$f" ] || continue
    "$TCC" -c -DMINIMUM_SET -Dcu_NO_EXPLICIT -I ../../sample/cppunit "$f" \
        -o "$WORK/cc.o" >/dev/null 2>"$LOG"
    ec=$?
    if is_crash $ec "$LOG"; then
        echo "  [CRASH] cppunit corpus: $(basename "$f") exit=$ec"; cat "$LOG"
        CRASHES=$((CRASHES + 1)); FAILED=$((FAILED + 1))
    fi
done
echo "=== crash corpus: done (crashes so far: $CRASHES) ==="

# === Phase 5: G7 CPPUnit machine gate (port of build_cppunit.bat) ===
# The baseline-integrity half of the .bat (git diff against
# cppunit-original-base) is platform-neutral and stays on the Windows gate.
echo "=== CPPUnit G7 ==="
CU="$ROOT/sample/cppunit"
CUOUT="$WORK/cppunit"
mkdir -p "$CUOUT"
G7=0
for s in RepeatedTest SimpleList SimpleString TestCase TestDecorator \
         TestFailure TestRegistry TestResult TestRunner TestSetup TestSuite \
         TestUtility all_test_tpp; do
    if ! "$TCC" -c -DMINIMUM_SET -Dcu_NO_EXPLICIT -I "$CU" -w "$CU/$s.cpp" \
            -o "$CUOUT/$s.o" >"$LOG" 2>&1; then
        echo "  [G7 FAIL] compile: $s.cpp"; cat "$LOG"; G7=$((G7 + 1))
    fi
done
if "$TCC" "$CUOUT"/*.o -o "$CUOUT/all_test_tpp" >"$LOG" 2>&1; then
    "$CUOUT/all_test_tpp" >"$CUOUT/run.log" 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "  [G7 FAIL] executable exit code $rc"; G7=$((G7 + 1))
    fi
    for line in TESTS:17 FAILURES:0 ERRORS:0 \
        PASS:test_case_lifecycle PASS:test_case_failure_record \
        PASS:test_assert_equals PASS:test_result_counts \
        PASS:test_failure_detail PASS:test_suite_run PASS:test_registry \
        PASS:test_runner_run PASS:test_repeated PASS:test_decorator \
        PASS:test_setup_hooks PASS:test_listener PASS:test_simple_string \
        PASS:test_simple_string_grow PASS:test_simple_list \
        PASS:test_simple_list_iter PASS:test_auto_ptr; do
        if ! grep -x -q -- "$line" "$CUOUT/run.log"; then
            echo "  [G7 FAIL] missing exact line: $line"; G7=$((G7 + 1))
        fi
    done
else
    echo "  [G7 FAIL] link"; cat "$LOG"; G7=$((G7 + 1))
fi
echo "=== CPPUnit G7 summary: $G7 failure(s) ==="
FAILED=$((FAILED + G7))

echo "=== run_all_mac summary: $FAILED gating failure(s), $CRASHES crash(es) ==="
[ $FAILED -eq 0 ]
