// BUG-20 (case M): a global ctor argument that names a shadowed class.
// Declaring the variable first suppresses the implicit typedef injection, so
// the struct tag stays reachable through struct_find() only.  With the old
// unconditional struct_find() in cpp_tok_starts_type_name(), `Foo g(tex);`
// was taken for a function declaration and compiled silently - the error only
// surfaced later at `g.a` ("lvalue expected"), which is why this test must
// RUN and check the value rather than merely compile.
int tex = 5;

struct tex { float u, v; };

struct Foo {
    int a;
    Foo(int x) { a = x; }
};

Foo g(tex);

// control: same shape without any shadowing
int val = 5;
Foo g2(val);

int main(void)
{
    if (g.a != 5)
        return 1;
    if (g2.a != 5)
        return 1;
    return 0;
}
