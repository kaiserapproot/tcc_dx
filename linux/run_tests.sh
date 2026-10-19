#!/bin/sh
# Linux (WSL) port of the core phases of dev/test/run_all.bat.
#
#   Phase 0  : linux/tests smoke (C, varargs, double ABI, C++, -run)
#   Phase 1  : compile-only (-c) of the same sources run_all.bat gates
#   Phase 2  : build + run (exit code 0 expected)
#   Phase 2b : -run a9/local_static_dtor.cpp
#   Phase 3  : a9/negative (must be rejected, with the .expected text)
#
# The Windows-only gates (*.bat under a9/manual, sdk/dwarf/asm/simd, PE
# TLS / startup) are not ported.  A crash is a signal exit (>= 128) or
# "internal error" on stderr; it is reported once and never retried.

HERE=$(cd "$(dirname "$0")" && pwd)
TCC=${TCC_EXE:-$HERE/out/tcc}
T=$HERE/../dev/test
WORK=${TMPDIR:-/tmp}/tpp_linux_tests
KNOWNFAIL=" sm3.cpp sm6.cpp t_assign.cpp "
# Windows-only sources (include windows.h / Win32 API): not run on Linux.
WINONLY=" crt_header_compat.cpp "
FAILED=0
CRASHES=0
LOG=$WORK/err.log

mkdir -p "$WORK/obj" "$WORK/exe" "$WORK/neg"

is_crash() { # $1 = exit code, $2 = log
    [ "$1" -ge 128 ] && return 0
    grep -q "internal error" "$2" 2>/dev/null
}

crash() {
    echo "  [CRASH] $1 exit=$2"
    cat "$LOG"
    CRASHES=$((CRASHES + 1))
    FAILED=$((FAILED + 1))
}

echo "=== Phase 0: linux/tests ==="
for f in "$HERE"/tests/*.c "$HERE"/tests/*.cpp; do
    [ -f "$f" ] || continue
    n=$(basename "$f")
    exe=$WORK/exe/lt_${n%.*}
    "$TCC" "$f" -o "$exe" -lm >/dev/null 2>"$LOG"; ec=$?
    if is_crash $ec "$LOG"; then crash "$n" $ec; continue; fi
    if [ $ec -ne 0 ]; then
        cat "$LOG"; echo "  [BUILD FAIL] $n"; FAILED=$((FAILED + 1)); continue
    fi
    if ! "$exe" >"$WORK/out.txt" 2>&1; then
        cat "$WORK/out.txt"; echo "  [RUN FAIL] $n"; FAILED=$((FAILED + 1)); continue
    fi
    "$TCC" -run "$f" >/dev/null 2>"$LOG"; ec=$?
    if is_crash $ec "$LOG"; then crash "-run $n" $ec; continue; fi
    if [ $ec -ne 0 ]; then
        cat "$LOG"; echo "  [-run FAIL] $n"; FAILED=$((FAILED + 1))
    fi
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
    "$TCC" -c "$f" -o "$WORK/obj/${n%.*}.o" >/dev/null 2>"$LOG"; ec=$?
    if is_crash $ec "$LOG"; then crash "$f" $ec; continue; fi
    if [ $ec -ne 0 ]; then
        case "$KNOWNFAIL" in
        *" $n "*) echo "  [known gap, non-gating] $f" ;;
        *) cat "$LOG"; echo "  [COMPILE FAIL] $f"; FAILED=$((FAILED + 1)) ;;
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
    "$TCC" "$f" -o "$exe" >/dev/null 2>"$LOG"; ec=$?
    if is_crash $ec "$LOG"; then crash "$f" $ec; continue; fi
    if [ $ec -ne 0 ]; then
        cat "$LOG"; echo "  [BUILD FAIL] $f"; FAILED=$((FAILED + 1)); continue
    fi
    "$exe" >/dev/null 2>&1; ec=$?
    if [ $ec -ne 0 ]; then
        echo "  [RUN FAIL] $f exit=$ec"; FAILED=$((FAILED + 1))
    fi
done

echo "=== Phase 2b: -run a9/local_static_dtor.cpp ==="
"$TCC" -run a9/local_static_dtor.cpp >"$LOG" 2>&1; ec=$?
if is_crash $ec "$LOG"; then crash "-run a9/local_static_dtor.cpp" $ec
elif [ $ec -ne 0 ]; then
    cat "$LOG"; echo "  [RUN FAIL] -run a9/local_static_dtor.cpp"
    FAILED=$((FAILED + 1))
fi

echo "=== Phase 3: a9/negative ==="
for f in a9/negative/*.cpp; do
    [ -f "$f" ] || continue
    n=$(basename "$f" .cpp)
    nlog=$WORK/neg/$n.log
    "$TCC" -c "$f" -o "$WORK/neg/$n.o" >"$nlog" 2>&1; ec=$?
    if is_crash $ec "$nlog"; then cp "$nlog" "$LOG"; crash "$f" $ec; continue; fi
    if [ $ec -eq 0 ]; then
        echo "  [NEGATIVE FAIL] $f compiled but was expected to be rejected"
        FAILED=$((FAILED + 1))
    elif [ -f "a9/negative/$n.expected" ]; then
        pat=$(tr -d '\r' < "a9/negative/$n.expected" | tail -n 1)
        if ! grep -qF "$pat" "$nlog"; then
            pat8=$(tr -d '\r' < "a9/negative/$n.expected" | iconv -f CP932 -t UTF-8 2>/dev/null | tail -n 1)
            if [ -z "$pat8" ] || ! grep -qF "$pat8" "$nlog"; then
                echo "  [NEGATIVE FAIL] $f rejected, but not with \"$pat\""
                cat "$nlog"
                FAILED=$((FAILED + 1))
            fi
        fi
    fi
done

echo "=== Phase 4: X11 + GLX window from C++ (skipped without a display) ==="
f=$HERE/tests/gl/x11_glx.cpp
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "  [skip] no DISPLAY"
elif [ ! -f /usr/include/X11/Xlib.h ] || [ ! -f /usr/include/GL/glx.h ]; then
    echo "  [skip] libx11-dev / libgl-dev not installed"
else
    "$TCC" "$f" -o "$WORK/exe/x11_glx" -lX11 -lGL >/dev/null 2>"$LOG"; ec=$?
    if is_crash $ec "$LOG"; then crash x11_glx.cpp $ec
    elif [ $ec -ne 0 ]; then
        cat "$LOG"; echo "  [BUILD FAIL] x11_glx.cpp"; FAILED=$((FAILED + 1))
    else
        timeout 30 "$WORK/exe/x11_glx" >"$WORK/out.txt" 2>&1; ec=$?
        if [ $ec -eq 77 ]; then
            echo "  [skip] cannot open display"
        elif [ $ec -ne 0 ]; then
            cat "$WORK/out.txt"; echo "  [RUN FAIL] x11_glx.cpp exit=$ec"
            FAILED=$((FAILED + 1))
        else
            grep "GL_RENDERER" "$WORK/out.txt"
        fi
    fi
fi

echo "=== run_tests summary: $FAILED gating failure(s), $CRASHES crash(es) ==="
[ $FAILED -eq 0 ]
