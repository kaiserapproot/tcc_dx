/* BUG-20 (case D): the C non-regression baseline.  In C, tags and ordinary
   identifiers live in separate namespaces, so all of this is plainly legal
   and must keep compiling exactly as before - the fix is C++ only and is
   gated on tcc_state->cpp.  This file is a .c on purpose; run_all.bat was
   extended to enumerate a9\*.c so it is actually gated. */
struct tex { float u, v; };

int tex = 5;

struct T { unsigned int id; };

static void free_tex(struct T *tex)
{
    tex->id = 0;
}

int main(void)
{
    struct T t;
    struct tex uv;

    t.id = 1;
    free_tex(&t);
    if (t.id != 0)
        return 1;
    uv.u = 1.0f;
    uv.v = 2.0f;
    if (uv.u + uv.v != 3.0f)
        return 1;
    if (tex != 5)
        return 1;
    return 0;
}
