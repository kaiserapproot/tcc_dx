struct M {
    int v;
    M() { v = 1; }
    M& operator=(const M& o) { v = o.v + 1000; return *this; }
};

struct H {
    M m;
    int n;
    H() { n = 0; }
    ~H() {}
};

int main()
{
    H x, y;

    x.m.v = 42;
    y = x;
    return y.m.v == 1042 ? 0 : 1;
}
