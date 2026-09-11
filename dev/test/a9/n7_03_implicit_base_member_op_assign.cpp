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

    x.m.v = 7;
    y = x;
    return y.m.v == 1007 && y.d == 0 ? 0 : 1;
}
