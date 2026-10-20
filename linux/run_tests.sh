#!/bin/sh
# Linux (WSL) port of the core phases of dev/test/run_all.bat, plus the
# ELF-specific checks the Windows suite does not have.
#
#   Phase 0  : linux/tests/*.c, *.cpp built with -o and run, and run with -run
#   Phase 1  : compile-only (-c) of the same sources run_all.bat gates
#   Phase 2  : build + run (exit code 0 expected)
#   Phase 2b : -run a9/local_static_dtor.cpp
#   Phase 3  : a9/negative (must be rejected, with the .expected text)
#   Phase 4  : X11 + GLX window from C++ (skipped without a display)
#   Phase 5  : ELF linking: multi-TU C / C++, static library, shared
#              library (-shared, -c then -shared), dlopen / dlclose x2,
#              link-time .so; C++ destructor runtime in a .so is rejected
#   Phase 6  : exit order (return / exit(), -o / -run) against glibc
#   Phase 7  : SysV ABI cross check, gcc objects <-> tpp objects
#
# The Windows-only gates (*.bat under a9/manual, sdk/dwarf/asm/simd, PE
# TLS / startup) are not ported.  A crash is a signal exit (>= 128) of
# the compiler or of a generated program, a timeout, or "internal error"
# on the compiler's stderr; it is reported once and never retried.

HERE=$(cd "$(dirname "$0")" && pwd)
TCC=${TCC_EXE:-$HERE/out/tcc}
T=$HERE/../dev/test
LT=$HERE/tests
KNOWNFAIL=" sm3.cpp sm6.cpp t_assign.cpp "
# Windows-only sources (include windows.h / Win32 API): not run on Linux.
WINONLY=" crt_header_compat.cpp "
FAILED=0
CRASHES=0
GCC=$(command -v gcc)

# Per-run work directory: parallel runs must not share logs or binaries.
# Created under linux/out (build output, not tracked) unless
# TPP_TEST_TMPDIR names another place.
mkdir -p "${TPP_TEST_TMPDIR:-$HERE/out}" || exit 1
WORK=$(mktemp -d "${TPP_TEST_TMPDIR:-$HERE/out}/test.XXXXXX") || exit 1
trap 'rm -rf "$WORK"' EXIT
trap 'exit 130' INT TERM
LOG=$WORK/err.log
OUT=$WORK/out.txt
mkdir -p "$WORK/obj" "$WORK/exe" "$WORK/neg" "$WORK/link" "$WORK/abi"

fail() { # $1 = message; the caller already printed any log
    echo "  [$1]"
    FAILED=$((FAILED + 1))
}

crash() { # $1 = what, $2 = exit code, $3 = log to show
    echo "  [CRASH] $1 exit=$2"
    [ -n "$3" ] && cat "$3"
    CRASHES=$((CRASHES + 1))
    FAILED=$((FAILED + 1))
}

# Run the compiler.  Returns 0 on success; on failure it has already
# counted the failure (crash or plain error).  $1 = label, rest = args.
cc_tpp() {
    label=$1; shift
    "$TCC" "$@" >"$LOG" 2>&1; ec=$?
    if [ $ec -ge 128 ] || grep -q "internal error" "$LOG" 2>/dev/null; then
        crash "tcc $label" $ec "$LOG"; return 1
    fi
    if [ $ec -ne 0 ]; then
        cat "$LOG"; fail "BUILD FAIL $label"; return 1
    fi
    return 0
}

# Run a generated program (or tcc -run) with a timeout; stdout+stderr go
# to $OUT.  Returns its exit code.  Exit codes 124 and up are counted here
# and never reach the caller as a plain result: 124 = timeout (crash),
# 125-127 = timeout could not run the program (failure), >= 128 = signal
# (crash).  1-123 are the program's own result; the caller decides.
run_prog() { # $1 = label, rest = command
    label=$1; shift
    timeout 30 "$@" >"$OUT" 2>&1; ec=$?
    if [ $ec -eq 124 ]; then
        cat "$OUT"; crash "$label (timeout)" $ec ""
    elif [ $ec -ge 125 ] && [ $ec -le 127 ]; then
        cat "$OUT"; fail "EXEC FAIL $label exit=$ec (could not run)"
    elif [ $ec -ge 128 ]; then
        cat "$OUT"; crash "$label" $ec ""
    fi
    return $ec
}

# Exit code 0 required; 1-123 counted here, 124+ already by run_prog.
expect_run_ok() { # $1 = label, rest = command
    run_prog "$@"; ec=$?
    [ $ec -eq 0 ] && return 0
    [ $ec -lt 124 ] && { cat "$OUT"; fail "RUN FAIL $1 exit=$ec"; }
    return 1
}

echo "=== Phase 0: linux/tests ==="
for f in "$LT"/*.c "$LT"/*.cpp; do
    [ -f "$f" ] || continue
    n=$(basename "$f")
    exe=$WORK/exe/lt_${n%.*}
    cc_tpp "$n" "$f" -o "$exe" -lm || continue
    expect_run_ok "$n" "$exe" || continue
    expect_run_ok "-run $n" "$TCC" -run "$f"
done

cd "$T" || exit 1

echo "=== Phase 1: compile-only ==="
for f in smoke/*.c smoke/*.cpp a2/*.c a2/*.cpp a3/*.c a3/*.cpp a4/*.cpp \
         a5/*.cpp a6/*.cpp a8/*.cpp a9/*.c a9/*.cpp \
         a7/member_call.cpp a7/default_arg.cpp a7/inline_member.cpp \
         a7/typedef_class.cpp; do
    [ -f "$f" ] || continue
    n=$(basename "$f")
    case "$WINONLY" in *" $n "*) continue ;; esac
    "$TCC" -c "$f" -o "$WORK/obj/${n%.*}.o" >"$LOG" 2>&1; ec=$?
    if [ $ec -ge 128 ] || grep -q "internal error" "$LOG"; then
        crash "$f" $ec "$LOG"; continue
    fi
    if [ $ec -ne 0 ]; then
        case "$KNOWNFAIL" in
        *" $n "*) echo "  [known gap, non-gating] $f" ;;
        *) cat "$LOG"; fail "COMPILE FAIL $f" ;;
        esac
    fi
done

echo "=== Phase 2: build + run ==="
for f in a9/*.cpp a7/member_call.cpp a7/default_arg.cpp \
         a7/inline_member.cpp a7/typedef_class.cpp smoke/hello.c; do
    [ -f "$f" ] || continue
    n=$(basename "$f")
    case "$WINONLY" in *" $n "*) continue ;; esac
    exe=$WORK/exe/${n%.*}
    cc_tpp "$f" "$f" -o "$exe" || continue
    expect_run_ok "$f" "$exe"
done

echo "=== Phase 2b: -run a9/local_static_dtor.cpp ==="
expect_run_ok "-run a9/local_static_dtor.cpp" "$TCC" -run a9/local_static_dtor.cpp

echo "=== Phase 3: a9/negative ==="
NOEXP=""
for f in a9/negative/*.cpp; do
    [ -f "$f" ] || continue
    n=$(basename "$f" .cpp)
    nlog=$WORK/neg/$n.log
    exp=a9/negative/$n.expected
    "$TCC" -c "$f" -o "$WORK/neg/$n.o" >"$nlog" 2>&1; ec=$?
    if [ $ec -ge 128 ] || grep -q "internal error" "$nlog"; then
        crash "$f" $ec "$nlog"; continue
    fi
    if [ $ec -eq 0 ]; then
        fail "NEGATIVE FAIL $f compiled but was expected to be rejected"
        continue
    fi
    if [ ! -f "$exp" ]; then
        # No .expected: only the rejection (exit code) is checked, as in
        # run_all.bat.  Listed below so the weaker check stays visible.
        NOEXP="$NOEXP $n"
        continue
    fi
    # The .expected files are CP932 (Windows messages); tcc on Linux
    # prints UTF-8, so try the pattern in both encodings.
    pat=$(tr -d '\r' < "$exp" | tail -n 1)
    pat8=$(tr -d '\r' < "$exp" | iconv -f CP932 -t UTF-8 2>/dev/null | tail -n 1)
    if [ -z "$pat" ]; then
        fail "NEGATIVE FAIL $exp is empty"
    elif ! grep -qF "$pat" "$nlog" && { [ -z "$pat8" ] || ! grep -qF "$pat8" "$nlog"; }; then
        cat "$nlog"
        fail "NEGATIVE FAIL $f rejected, but not with \"$pat\""
    fi
done
[ -n "$NOEXP" ] && echo "  [exit-code only, no .expected]$NOEXP"

echo "=== Phase 4: X11 + GLX window from C++ (skipped without a display) ==="
f=$LT/gl/x11_glx.cpp
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "  [skip] no DISPLAY"
elif [ ! -f /usr/include/X11/Xlib.h ] || [ ! -f /usr/include/GL/glx.h ]; then
    echo "  [skip] libx11-dev / libgl-dev not installed"
elif cc_tpp x11_glx.cpp "$f" -o "$WORK/exe/x11_glx" -lX11 -lGL; then
    run_prog x11_glx.cpp "$WORK/exe/x11_glx"; ec=$?
    if [ $ec -eq 77 ]; then
        echo "  [skip] cannot open display"
    elif [ $ec -eq 0 ]; then
        grep "GL_RENDERER" "$OUT"
    elif [ $ec -lt 124 ]; then
        cat "$OUT"; fail "RUN FAIL x11_glx.cpp exit=$ec"
    fi
fi

echo "=== Phase 5: ELF linking ==="
L=$LT/link
W=$WORK/link
# multi-TU C + static library made with tcc -ar
if cc_tpp a_main.c -c "$L/a_main.c" -o "$W/a_main.o" \
   && cc_tpp b_part.c -c "$L/b_part.c" -o "$W/b_part.o" \
   && cc_tpp c_lib.c -c "$L/c_lib.c" -o "$W/c_lib.o" \
   && cc_tpp "-ar libc_lib.a" -ar rcs "$W/libc_lib.a" "$W/c_lib.o" \
   && cc_tpp "link C objects + .a" "$W/a_main.o" "$W/b_part.o" -L"$W" -lc_lib -o "$W/c_multi"; then
    expect_run_ok "multi-TU C + static lib" "$W/c_multi"
fi
# multi-TU C++: class, global ctor/dtor and overloads across TUs
if cc_tpp m_class.cpp -c "$L/m_class.cpp" -o "$W/m_class.o" \
   && cc_tpp m_main.cpp -c "$L/m_main.cpp" -o "$W/m_main.o" \
   && cc_tpp "link C++ objects" "$W/m_main.o" "$W/m_class.o" -o "$W/cpp_multi"; then
    if expect_run_ok "multi-TU C++" "$W/cpp_multi" && ! grep -q "M_GLOBAL_DTOR" "$OUT"; then
        cat "$OUT"; fail "RUN FAIL multi-TU C++: global dtor did not run"
    fi
fi
# shared library, built directly and via -c, loaded twice with dlopen
if cc_tpp "dl_host.c" "$L/dl_host.c" -o "$W/dl_host" -ldl; then
    for mode in direct objects; do
        so=$W/libso_$mode.so
        if [ $mode = direct ]; then
            cc_tpp "-shared so_lib.c" -shared -fPIC "$L/so_lib.c" -o "$so" || continue
        else
            cc_tpp "-c so_lib.c" -fPIC -c "$L/so_lib.c" -o "$W/so_lib.o" || continue
            cc_tpp "-shared so_lib.o" -shared "$W/so_lib.o" -o "$so" || continue
        fi
        if expect_run_ok "dlopen x2 ($mode)" "$W/dl_host" "$so"; then
            nfini=$(grep -c "SO_FINI" "$OUT")
            if [ "$nfini" != 2 ] || ! tail -n 1 "$OUT" | grep -q HOST_END; then
                cat "$OUT"; fail "RUN FAIL dlopen x2 ($mode): .fini_array not run once per dlclose"
            fi
        fi
    done
fi
# C .so that calls atexit(): the handler must run at dlclose (through
# __cxa_finalize from linux/lib/dsohandle.c), not after the unmapping.
# Built directly and via -c, loaded and closed twice.
WANT_AT="CLOSING SO_ATEXIT CLOSED CLOSING SO_ATEXIT CLOSED"
if cc_tpp dl_host_atexit.c "$L/dl_host_atexit.c" -o "$W/dl_host_atexit" -ldl; then
    for mode in direct objects; do
        so=$W/libso_atexit_$mode.so
        if [ $mode = direct ]; then
            cc_tpp "-shared so_atexit.c" -shared -fPIC "$L/so_atexit.c" -o "$so" || continue
        else
            cc_tpp "-c so_atexit.c" -fPIC -c "$L/so_atexit.c" -o "$W/so_atexit.o" || continue
            cc_tpp "-shared so_atexit.o" -shared "$W/so_atexit.o" -o "$so" || continue
        fi
        if expect_run_ok "atexit in .so, dlopen x2 ($mode)" "$W/dl_host_atexit" "$so"; then
            got=$(tr '\n' ' ' < "$OUT" | sed 's/ *$//')
            if [ "$got" != "$WANT_AT" ]; then
                fail "RUN FAIL atexit in .so ($mode): got \"$got\", want \"$WANT_AT\""
            fi
        fi
    done
fi
# link-time .so (-L -l) + LD_LIBRARY_PATH
if [ -f "$W/libso_direct.so" ] \
   && cc_tpp so_user.c "$L/so_user.c" -L"$W" -lso_direct -o "$W/so_user"; then
    expect_run_ok "link-time .so" env LD_LIBRARY_PATH="$W" "$W/so_user"
fi
# C++ shared library without destructor runtime: global ctor in the .so
if cc_tpp "-shared so_cpp.cpp" -shared -fPIC "$L/so_cpp.cpp" -o "$W/libso_cpp.so" \
   && cc_tpp dl_host_cpp.c "$L/dl_host_cpp.c" -o "$W/dl_host_cpp" -ldl; then
    expect_run_ok "dlopen x2 C++ .so" "$W/dl_host_cpp" "$W/libso_cpp.so"
fi
# C++ global with a destructor in a .so: rejected on both paths
for mode in direct objects; do
    if [ $mode = direct ]; then
        "$TCC" -shared -fPIC "$L/so_cpp_dtor.cpp" -o "$W/libbad.so" >"$LOG" 2>&1; ec=$?
    else
        "$TCC" -fPIC -c "$L/so_cpp_dtor.cpp" -o "$W/bad.o" >"$LOG" 2>&1 \
            && "$TCC" -shared "$W/bad.o" -o "$W/libbad.so" >"$LOG" 2>&1; ec=$?
    fi
    if [ $ec -ge 128 ] || grep -q "internal error" "$LOG"; then
        crash "C++ dtor in .so ($mode)" $ec "$LOG"
    elif [ $ec -eq 0 ]; then
        fail "NEGATIVE FAIL C++ dtor in .so ($mode) was accepted"
    elif ! grep -q "destructor runtime in DLL is unsupported" "$LOG"; then
        cat "$LOG"; fail "NEGATIVE FAIL C++ dtor in .so ($mode): unexpected error"
    fi
done

echo "=== Phase 6: exit order ==="
EXPECT_ORDER="MAIN ATEXIT GLOBAL_DTOR FINI_ARRAY"
if cc_tpp exit_order.cpp "$LT/exit_order.cpp" -o "$WORK/exe/exit_order"; then
    for how in return exit; do
        arg=; [ $how = exit ] && arg=exit
        for mode in exe run; do
            if [ $mode = exe ]; then
                run_prog "exit order ($mode, $how)" "$WORK/exe/exit_order" $arg
            else
                run_prog "exit order ($mode, $how)" "$TCC" -run "$LT/exit_order.cpp" $arg
            fi
            ec=$?
            [ $ec -ge 124 ] && continue
            got=$(tr '\n' ' ' < "$OUT" | sed 's/ *$//')
            if [ $ec -ne 0 ]; then
                fail "RUN FAIL exit order ($mode, $how) exit=$ec, output \"$got\""
            elif [ "$got" != "$EXPECT_ORDER" ]; then
                fail "ORDER FAIL ($mode, $how): got \"$got\", want \"$EXPECT_ORDER\""
            fi
        done
    done
fi

echo "=== Phase 7: SysV ABI cross check with gcc ==="
A=$LT/abi
W=$WORK/abi
if [ -z "$GCC" ]; then
    echo "  [skip] gcc not found"
else
    "$GCC" -c -O1 -I"$A" "$A/abi_impl.c" -o "$W/impl_gcc.o" \
        && "$GCC" -c -O1 -I"$A" "$A/abi_check.c" -o "$W/check_gcc.o" \
        || fail "BUILD FAIL abi (gcc)"
    cc_tpp "abi_impl.c" -c -I"$A" "$A/abi_impl.c" -o "$W/impl_tpp.o"
    cc_tpp "abi_check.c" -c -I"$A" "$A/abi_check.c" -o "$W/check_tpp.o"
    for pair in "tpp:gcc" "gcc:tpp" "tpp:tpp"; do
        caller=${pair%%:*}; callee=${pair##*:}
        [ -f "$W/check_$caller.o" ] && [ -f "$W/impl_$callee.o" ] || continue
        exe=$W/abi_${caller}_calls_$callee
        # link with gcc (GNU ld) when a gcc object is involved, which also
        # checks that tpp objects are consumable by an external linker;
        # tpp objects need libtcc1.a (__va_arg etc.)
        if [ $caller = tpp ] && [ $callee = tpp ]; then
            cc_tpp "link abi tpp:tpp" "$W/check_tpp.o" "$W/impl_tpp.o" -o "$exe" || continue
        else
            "$GCC" -no-pie "$W/check_$caller.o" "$W/impl_$callee.o" "$HERE/out/libtcc1.a" \
                -o "$exe" >"$LOG" 2>&1 \
                || { cat "$LOG"; fail "BUILD FAIL link abi $pair"; continue; }
        fi
        expect_run_ok "abi caller=$caller callee=$callee" "$exe"
    done
    # structs whose eightbytes mix SSE and INTEGER: tpp cannot pass them
    # like gcc, so every by-value use must be rejected; CASE 0 (by pointer,
    # 8-byte float+int struct) must still compile
    for c in 0 1 2 3 4; do
        "$TCC" -c -DCASE=$c "$A/abi_mixed_reject.c" -o "$W/mixed_$c.o" >"$LOG" 2>&1; ec=$?
        if [ $ec -ge 128 ] || grep -q "internal error" "$LOG"; then
            crash "abi_mixed_reject CASE=$c" $ec "$LOG"
        elif [ $c = 0 ]; then
            [ $ec -eq 0 ] || { cat "$LOG"; fail "COMPILE FAIL abi_mixed_reject CASE=0"; }
        elif [ $ec -eq 0 ]; then
            fail "NEGATIVE FAIL abi_mixed_reject CASE=$c was accepted"
        elif ! grep -q "eightbytes mix SSE and INTEGER" "$LOG"; then
            cat "$LOG"; fail "NEGATIVE FAIL abi_mixed_reject CASE=$c: unexpected error"
        fi
    done
fi

echo "=== run_tests summary: $FAILED gating failure(s), $CRASHES crash(es) ==="
[ $FAILED -eq 0 ]
