#!/bin/sh
# build_opengl_mac.sh - build_opengl.bat の macOS 版。
#
#   sh build_opengl_mac.sh              main_glut.c をビルド（GLUT 版）
#   sh build_opengl_mac.sh run          ビルドして起動
#   sh build_opengl_mac.sh cocoa        main_cocoa_gl.c をビルド
#                                       （Cocoa + NSOpenGLContext + CVDisplayLink を C だけで書いた版）
#   sh build_opengl_mac.sh cocoa run    ビルドして起動
#
# Windows は opengl32 / glut32 の .def でリンクするが、macOS は SDK の
# フレームワークを使う。tcc は clang の -framework に対応していないので:
#   ヘッダ:   -I build/macos/frameworks   （<OpenGL/gl.h> や <GLUT/glut.h> が見つかる）
#   リンク:   <SDK>/System/Library/Frameworks/<名前>.framework/<名前>.tbd を入力に渡す
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
TCC_ROOT="$ROOT/build/macos"

# tcc_set.bat を呼ぶ build_opengl.bat と同じく、未ビルドならビルドする
if [ ! -x "$TCC_ROOT/tcc" ] || [ ! -d "$TCC_ROOT/frameworks/GLUT" ]; then
    "$ROOT/build_mac.sh" --no-test
fi

SDK=$(xcrun --show-sdk-path)
FW="$SDK/System/Library/Frameworks"

target=glut
if [ "${1:-}" = "cocoa" ]; then
    target=cocoa
    shift
fi

# OpenGL / GLUT は macOS 10.14 から非推奨のため、警告を -w で抑える
if [ $target = glut ]; then
    out="$HERE/main_glut"
    "$TCC_ROOT/tcc" -w -I "$TCC_ROOT/frameworks" "$HERE/main_glut.c" \
        "$FW/OpenGL.framework/OpenGL.tbd" \
        "$FW/GLUT.framework/GLUT.tbd" \
        -o "$out"
else
    out="$HERE/main_cocoa_gl"
    "$TCC_ROOT/tcc" -w -I "$TCC_ROOT/frameworks" "$HERE/main_cocoa_gl.c" \
        "$FW/Cocoa.framework/Cocoa.tbd" \
        "$FW/OpenGL.framework/OpenGL.tbd" \
        "$FW/CoreVideo.framework/CoreVideo.tbd" \
        "$SDK/usr/lib/libobjc.tbd" \
        -o "$out"
fi
echo "built: $out"

if [ "${1:-}" = "run" ]; then
    shift
    "$out" "$@"
fi
