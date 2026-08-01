/* C++ の inline 関数は「使われた場合にのみ実体化される」（vague linkage）。
 * 非 inline のプロトタイプが先行していても同じでなければならない。
 *
 * 回帰の内容: 以前は先行プロトタイプがあると patch_type で VT_INLINE が
 * 失われ、未使用でも本体が即座にコンパイルされていた。windows.h の
 * FORCEINLINE（C++ では素の inline に展開される）で顕在化し、
 * NtCurrentTeb が実体化して __readgsqword の asm がエラーになっていた。
 *
 * もし実体化されてしまうと undefined_helper への未定義参照でリンクに失敗する。
 */
int undefined_helper(int x);          /* 宣言のみ。定義しない */

int unused_inline(int x);             /* 非 inline の先行プロトタイプ */
inline __attribute__((__always_inline__))
int unused_inline(int x) { return undefined_helper(x); }

/* プロトタイプ無しの素の inline も同様 */
inline int unused_inline2(int x) { return undefined_helper(x) + 1; }

#include <stdio.h>

int main(void)
{
    printf("OK\n");
    return 0;
}
