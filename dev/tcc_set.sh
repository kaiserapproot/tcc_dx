# tcc_set.sh - tcc_set.bat の macOS 版。tpp の tcc を PATH に追加する。
#
#   source dev/tcc_set.sh            # 未ビルドならビルドしてから設定する
#   source dev/tcc_set.sh --rebuild  # ソースを直したあと、ビルドし直してから設定する
#
# 設定後は tcc をそのまま使える。
#   tcc hello.c -o hello && ./hello
#   tcc foo.cpp bar.c -o app
#   tcc -run hello.c
#
# tcc_set.bat と違い、C_INCLUDE_PATH / CPATH / LIBRARY_PATH は設定しない。
# macOS では clang もこれらの環境変数を読むため、tcc 用の stdarg.h などが
# clang のビルド（build_mac.sh 自身を含む）に混ざって壊れる。
# macOS 版の tcc は、自分の置き場所の include/ と lib/libtcc1-64.a、
# Xcode の SDK を自動で探すので、これらの設定はいらない。

# source されたか（zsh / bash）と、スクリプト自身の場所
_tcc_set_sourced=0
if [ -n "${ZSH_VERSION:-}" ]; then
    case "${ZSH_EVAL_CONTEXT:-}" in *file*) _tcc_set_sourced=1 ;; esac
    _tcc_set_self=${(%):-%x}
elif [ -n "${BASH_VERSION:-}" ]; then
    [ "${BASH_SOURCE[0]}" != "$0" ] && _tcc_set_sourced=1
    _tcc_set_self=${BASH_SOURCE[0]}
else
    _tcc_set_self=$0
fi

# source せずに実行すると、PATH の設定が呼び出し元のシェルに残らない
if [ "$_tcc_set_sourced" = 0 ]; then
    echo "使い方: source $_tcc_set_self [--rebuild]" >&2
    echo "（sh で実行すると、PATH の設定が今のシェルに残りません）" >&2
    exit 1
fi

_tcc_set_repo=$(cd "$(dirname "$_tcc_set_self")/.." && pwd)
TCC_ROOT="$_tcc_set_repo/build/macos"

_tcc_set_ok=1
if [ "${1:-}" = "--rebuild" ] || [ ! -x "$TCC_ROOT/tcc" ] \
        || [ ! -f "$TCC_ROOT/lib/libtcc1-64.a" ]; then
    echo "tcc をビルドします: $_tcc_set_repo/build_mac.sh --no-test"
    mkdir -p "$_tcc_set_repo/build"
    if ! "$_tcc_set_repo/build_mac.sh" --no-test >"$_tcc_set_repo/build/build_mac.log" 2>&1; then
        echo "ビルドに失敗しました。ログ: $_tcc_set_repo/build/build_mac.log" >&2
        tail -n 20 "$_tcc_set_repo/build/build_mac.log" >&2
        _tcc_set_ok=0
    fi
fi

if [ "$_tcc_set_ok" = 1 ]; then
    TCC="$TCC_ROOT/tcc"
    export TCC_ROOT TCC
    case ":$PATH:" in
        *":$TCC_ROOT:"*) ;;
        *) PATH="$TCC_ROOT:$PATH"; export PATH ;;
    esac
    hash -r 2>/dev/null
    echo "TCC path added: $TCC_ROOT"
    echo "TCC: $TCC"
    "$TCC" -v
fi

unset _tcc_set_self _tcc_set_repo _tcc_set_ok
