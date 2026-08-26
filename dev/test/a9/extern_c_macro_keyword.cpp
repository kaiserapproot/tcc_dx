// BUG-40: a `#define` directly after an extern "C" block used to store
// its body with DEMOTED C++ keywords - the closing sequence consumed the
// next token BEFORE lowering lex_c, and when that next() crossed the
// `#define` line the macro body was lexed in C mode ("false" became an
// identifier).  cuconfig.h's `cu_CATCH_ALL if (false)` right after
// <stddef.h> hit exactly this (TestCase.cpp:88).
extern "C" {
int ext_c_fn(void);
}
#define MYFALSE false
#define MYCATCH if (false)
struct A {
    bool m;
    void f();
};
void A::f()
{
    {
        m = true;
    } MYCATCH {
        m = false;
    }
}
int ext_c_fn(void)
{
    return 3;
}
int main()
{
    A a;

    a.m = false;
    a.f();
    if (a.m != true)
        return 1;
    if (MYFALSE)
        return 2;
    if (ext_c_fn() != 3)
        return 3;
    return 0;
}
