// BUG-20 (cases A/H/I): a parameter named like a class hides that class,
// so a statement starting with that name is an expression, not a declaration.
// Reduced from amateras cross.h mmd_gl_free_texture(), which failed with
// "identifier expected" because parse_btype() resolved `tex` via struct_find()
// and ignored the parameter that shadows it.
struct tex { float u, v; };

struct T { unsigned int id; int w; };

// case A: member access through a shadowing pointer parameter
static void free_tex(T *tex)
{
    if (tex->id) {
        tex->id = 0;
    }
    tex->w = 0;
}

// case I: member assignment through a shadowing value parameter
static void zero_tex(T tex)
{
    tex.id = 0;
}

// case H: plain assignment to a shadowing scalar parameter
static int bump(int tex)
{
    tex = 3;
    return tex;
}

// case B: a local variable may shadow the class too
static int local_shadow(T *p)
{
    T *tex = p;
    tex->id = 7;
    return (int)tex->id;
}

int main(void)
{
    T t;
    t.id = 1;
    t.w = 2;
    free_tex(&t);
    zero_tex(t);
    if (t.id != 0 || t.w != 0)
        return 1;
    if (bump(0) != 3)
        return 1;
    t.id = 0;
    if (local_shadow(&t) != 7)
        return 1;
    return 0;
}
