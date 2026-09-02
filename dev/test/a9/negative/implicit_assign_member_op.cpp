// C++98 would give this class an implicit copy assignment that assigns its
// members MEMBERWISE, so M::operator= would run.  tpp copies a struct
// assignment with a flat memcpy instead, which would silently skip that
// operator= - a member owning a heap buffer would leak the destination's
// buffer and start sharing the source's.  Rejected rather than miscompiled.
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
    return y.m.v;
}