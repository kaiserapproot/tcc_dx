// BUG-46: an implicit copy of a derived object must invoke the copy
// constructor of its primary base subobject, not only walk base fields.
int copies;
int a;
int b;

struct B {
    int *p;
    B() { p = &a; }
    B(const B& rhs) {
        ++copies;
        p = &b;
        *p = *rhs.p;
    }
};

struct D : B {
    int x;
    D() : B() { x = 0; }
};

int main()
{
    D src;
    D *dst;

    copies = 0;
    a = 42;
    b = 0;
    src.p = &a;
    dst = new D(src);
    if (copies != 1)
        return 1;
    if (dst->p != &b)
        return 2;
    if (*dst->p != 42)
        return 3;
    delete dst;
    return 0;
}
