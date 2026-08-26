// BUG-46 negative: implicit copy reconstruction must not treat a
// non-primary base as if it started at offset zero.
struct A {
    int a;
};

struct B {
    int b;
};

struct D : A, B {
};

int main()
{
    D source;
    D *copy = new D(source);
    return copy->a + copy->b;
}
