#!/bin/sh
# build_mac.sh - build tpp (tcc + C++ subset) for macOS x86_64 and run the gates.
#
#   ./build_mac.sh            build + smoke + regression gates (run_all_mac.sh)
#   ./build_mac.sh --no-test  build only
#
# Output (relocatable, like dev\tcc.exe on Windows):
#   build/macos/tcc           the compiler
#   build/macos/include/      tcc's own headers (copied from include/)
#   build/macos/lib/          libtcc1-64.a (runtime support) - same layout as
#                             dev\lib on Windows; put your own .a / .dylib here
# System headers / libraries come from the Xcode SDK (xcrun --show-sdk-path).

set -eu

ROOT=$(cd "$(dirname "$0")" && pwd)
OUT="$ROOT/build/macos"
OBJ="$OUT/obj"
CC=${CC:-clang}

case "$(uname -s)/$(uname -m)" in
    Darwin/x86_64) ;;
    *) echo "build_mac.sh: only macOS x86_64 is supported (got $(uname -s)/$(uname -m))" >&2
       exit 1 ;;
esac

SDK=$(xcrun --show-sdk-path)
if [ ! -d "$SDK/usr/include" ]; then
    echo "build_mac.sh: SDK headers not found at $SDK/usr/include" >&2
    exit 1
fi

echo "========================================"
echo "Building tcc (x86_64 Darwin)"
echo "  SDK: $SDK"
echo "========================================"
rm -rf "$OBJ"
mkdir -p "$OBJ"

# Same translation units as tcc.vcxproj (ONE_SOURCE=0), with tccmacho.c in
# place of tccpe.c.  tcctools.c is #included by tcc.c.
SRCS="libtcc.c tcc.c tccasm.c tccdbg.c tccelf.c tccgen.c tccpp.c tccrun.c
      x86_64-gen.c x86_64-link.c i386-asm.c tccmacho.c"
CFLAGS="-O2 -g -Wall -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result
        -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable
        -Wno-unused-label -Wno-string-plus-int -Wno-deprecated-declarations
        -Wno-parentheses -Wno-misleading-indentation -Wno-dangling-else
        -fno-strict-aliasing -fwrapv"
DEFS="-DONE_SOURCE=0 -DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO"
# {B}/lib first, like {B}/lib on PE: libtcc1-64.a and user libraries live
# there.  The rest is tcc.h's non-PE default; the SDK's usr/lib is added at
# run time by tcc_add_macos_sdkpath().
LIBPATHS="{B}/lib:{B}:/usr/lib:/lib:/usr/local/lib"

# These sources are CP932 (see CLAUDE.md).  clang reads every source as
# UTF-8 and does not know CP932 double-byte characters: a trail byte 0x5C
# ('\', e.g. in "表" / "構" / "ソ") is taken as an escape and silently
# corrupts the string, and the diagnostics would print as mojibake on a
# UTF-8 terminal.  So compile UTF-8 copies (the files in the repository are
# never rewritten); #line keeps diagnostics and debug info on the original.
CP932_SRCS="tccgen.c tccpp.c tccasm.c libtcc.c"
GEN="$OBJ/src"
mkdir -p "$GEN"
for f in $CP932_SRCS; do
    printf '#line 1 "%s"\n' "$ROOT/$f" > "$GEN/$f"
    iconv -f CP932 -t UTF-8 "$ROOT/$f" >> "$GEN/$f"
done

for f in $SRCS; do
    src="$ROOT/$f"
    case " $CP932_SRCS " in *" $f "*) src="$GEN/$f" ;; esac
    $CC $CFLAGS $DEFS -I"$ROOT" "-DCONFIG_USR_INCLUDE=\"$SDK/usr/include\"" \
        "-DCONFIG_TCC_LIBPATHS=\"$LIBPATHS\"" \
        -c "$src" -o "$OBJ/${f%.c}.o"
done
$CC -o "$OUT/tcc" "$OBJ"/*.o -lm -ldl -lpthread

echo "========================================"
echo "Installing headers"
echo "========================================"
rm -rf "$OUT/include"
cp -R "$ROOT/include" "$OUT/include"

# tcc has no framework search, so <OpenGL/gl.h> / <GLUT/glut.h> cannot be
# found.  frameworks/<Name> -> <SDK>/.../<Name>.framework/Headers makes
# "-I build/macos/frameworks" resolve them; link with the framework's .tbd
# (see sample/opengl/build_opengl_mac.sh).
FW="$SDK/System/Library/Frameworks"
rm -rf "$OUT/frameworks"
mkdir -p "$OUT/frameworks"
for d in "$FW"/*.framework; do
    name=$(basename "$d" .framework)
    [ -d "$d/Headers" ] && ln -s "$d/Headers" "$OUT/frameworks/$name"
done

echo "========================================"
echo "Building libtcc1-64.a"
echo "========================================"
LOBJ="$OBJ/lib"
mkdir -p "$LOBJ"
"$OUT/tcc" -c "$ROOT/lib/libtcc1.c"        -o "$LOBJ/libtcc1.o"
"$OUT/tcc" -c "$ROOT/lib/va_list.c"        -o "$LOBJ/va_list.o"
"$OUT/tcc" -c "$ROOT/lib/alloca86_64.S"    -o "$LOBJ/alloca.o"
"$OUT/tcc" -c "$ROOT/lib/alloca86_64-bt.S" -o "$LOBJ/alloca-bt.o"
rm -rf "$OUT/lib" "$OUT/libtcc1-64.a"   # the latter: old layout
mkdir -p "$OUT/lib"
"$OUT/tcc" -ar rcs "$OUT/lib/libtcc1-64.a" "$LOBJ"/*.o

if [ "${1:-}" = "--no-test" ]; then
    echo "Build Completed (tests skipped)"
    exit 0
fi

TCC="$OUT/tcc"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/tcc_build_mac.XXXXXX")
trap 'rm -rf "$TMP"' EXIT

echo "========================================"
echo "Smoke tests"
echo "========================================"
"$TCC" -v
"$TCC" "$ROOT/dev/test/smoke/hello.c" -o "$TMP/hello"
"$TMP/hello"
"$TCC" -run "$ROOT/dev/test/smoke/hello.c"

echo "========================================"
echo "Regression: repro_double6"
echo "========================================"
"$TCC" "$ROOT/dev/test/repro_double6.c" -o "$TMP/repro_double6"
"$TMP/repro_double6"

echo "========================================"
echo "L1 tests: run_all_mac.sh"
echo "========================================"
TCC_EXE="$TCC" sh "$ROOT/dev/test/run_all_mac.sh"

echo "========================================"
echo "Build Completed"
echo "========================================"
