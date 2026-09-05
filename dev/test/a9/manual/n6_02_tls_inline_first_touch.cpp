// N6-02 REVIEW FIX-1: TLS class object whose FIRST (and only) access sites are
// compiled late, inside gen_inline_functions() (deferred inline member bodies).
//
// 11f4831 では cpp_finish_tls_ctors() が thunk emit と同時に cpp_tls_ctors を
// dynarray_reset していたため, その後 gen_inline_functions() でコンパイルされる
// inline body からの TLS access は cpp_find_tls_ctor_wrapper() が NULL を返し,
// ctor=NULL (zero-fill のみ) の誤コードになる.
//
// 2 経路を測る:
//   INLINE_FIRST_TOUCH : main は TLS `object` を直接触らない. class 内定義の
//                        inline member Toucher::first() だけが object に触る.
//                        object.value は ctor が書く 123 でなければならない.
//   COLD_NESTED        : main は TLS `cold` を一切触らない. 別 TLS `outer` の
//                        inline ctor Outer::Outer() が初めて cold を参照する.
//                        cold.value は 123 (ctor 済み) でなければならない.
#include <stdio.h>

static int g_p_ctor_calls;
static int g_outer_ctor_calls;

struct P {
    int value;
    P() { value = 123; ++g_p_ctor_calls; }
};

thread_local P object;
thread_local P cold;

struct Toucher {
    // class 内定義 -> 本体は TU 末尾 gen_inline_functions() で codegen される.
    int first() { return object.value; }
};

struct Outer {
    int copy_of_cold;
    // 同じく deferred inline body. cold への最初のアクセスはここ.
    Outer() { copy_of_cold = cold.value; ++g_outer_ctor_calls; }
};

thread_local Outer outer;

int main()
{
    Toucher t;
    int inline_first;
    int cold_via_outer;

    if (g_p_ctor_calls != 0 || g_outer_ctor_calls != 0)
        return 1;

    inline_first = t.first();
    if (inline_first != 123)
        return 2;
    if (g_p_ctor_calls != 1)
        return 3;
    if (object.value != 123 || g_p_ctor_calls != 1)
        return 4;

    cold_via_outer = outer.copy_of_cold;
    if (g_outer_ctor_calls != 1)
        return 5;
    if (cold_via_outer != 123)
        return 6;
    if (g_p_ctor_calls != 2)
        return 7;
    if (cold.value != 123 || g_p_ctor_calls != 2)
        return 8;

    printf("N6_02_INLINE_FIRST_TOUCH=PASS\n");
    printf("INLINE_FIRST_TOUCH_VALUE=%d\n", inline_first);
    printf("N6_02_COLD_NESTED_TLS_CTOR=PASS\n");
    printf("COLD_NESTED_VALUE=%d\n", cold_via_outer);
    printf("P_CTOR_CALLS=%d\n", g_p_ctor_calls);
    return 0;
}
