// Same guard, reached through a BASE subobject: the walker must recurse into
// base fields, not just the class's own data members.
struct M {
    int v;
    M() { v = 1; }
    M& operator=(const M& o) { v = o.v + 1000; return *this; }
};

struct B {
    M m;
    B() {}
    ~B() {}
};

struct D : public B {
    int d;
    D() { d = 0; }
    ~D() {}
};

int main()
{
    D x, y;

    y = x;
    return y.d;
}