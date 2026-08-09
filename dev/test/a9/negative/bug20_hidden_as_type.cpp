// NEGATIVE test: must FAIL to compile.
// While `tex` is hidden by the parameter, it is not a type name any more, so
// using it as one has to be rejected.  If the lookup helper ever falls back to
// struct_find() while an ordinary binding exists, this would start compiling
// and the shadowing fix would be silently undone.
struct tex { float u, v; };

struct T { unsigned int id; };

void f(T *tex)
{
    tex t2;      // expected error: `tex` here is the parameter, not the class
    t2.u = 1.0f;
    tex->id = 0;
}

int main(void)
{
    return 0;
}
