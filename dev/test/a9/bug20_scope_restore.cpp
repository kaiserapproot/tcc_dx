// BUG-20 negative-side guards: these already worked before the fix and must
// keep working, so they pin the behaviour the new lookup helper must NOT
// change (cases E/F/G/J/L and the forward-declaration fallback).
struct tex { float u, v; };

struct T { unsigned int id; };

struct Foo {
    int a;
    Foo(int x) { a = x; }
};

// case E: an elaborated-type-specifier still names the class while hidden
static float explicit_tag(T *tex)
{
    struct tex local;
    local.u = 1.5f;
    tex->id = 0;
    return local.u;
}

// case F: the class name is visible again once the inner scope ends
static float scope_restored(T *p)
{
    {
        T *tex = p;
        tex->id = 0;
    }
    tex t2;
    t2.u = 2.5f;
    return t2.u;
}

// case G: a local ctor argument that names a shadowing parameter
static int local_ctor(int tex)
{
    Foo lf(tex);
    return lf.a;
}

// case L: declaring a same-named variable in an inner scope is fine
static int inner_decl(void)
{
    int tex = 4;
    return tex;
}

// struct_find() fallback: a forward-declared struct pushes no implicit
// typedef, so the tag namespace is the only place it can be found.
struct FwdOnly;
static FwdOnly *g_fwd;

int main(void)
{
    T t;
    t.id = 1;
    if (explicit_tag(&t) != 1.5f)
        return 1;
    if (scope_restored(&t) != 2.5f)
        return 1;
    if (local_ctor(9) != 9)
        return 1;
    if (inner_decl() != 4)
        return 1;
    if (g_fwd != 0)
        return 1;
    return 0;
}
