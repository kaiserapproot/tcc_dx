// virtual returning a struct through the hidden pointer, reached via a
// non-primary base: the hidden pointer takes arg0 and moves `this` to arg1,
// but the SysV thunk adjusts arg0 (RDI).  It must be rejected, as on Linux,
// rather than silently corrupt the returned value.
struct Big { long a, b, c; };
struct Tracked {
    int v;
    Tracked() : v(0) {}
    Tracked(int x) : v(x) {}
    Tracked(const Tracked &o) : v(o.v) {}
    ~Tracked() {}
};
struct A { int pad; virtual int fa() { return 1; } };
struct B { int tag; virtual Big big() { Big r = { 0, 0, 0 }; return r; }
           virtual Tracked tr() { return Tracked(0); }
           virtual int plain() { return 0; } };
struct D : A, B {
    long k;
    D() : k(40) {}
    virtual Big big() { Big r = { k, k + 1, k + 2 }; return r; }
    virtual Tracked tr() { return Tracked((int)k + 3); }
    virtual int plain() { return (int)k + 4; }
};
int main()
{
    D d;
    B *p = &d;
    Big g = p->big();
    if (g.a != 40 || g.b != 41 || g.c != 42) return 1;
    Tracked t = p->tr();
    if (t.v != 43) return 2;
    if (p->plain() != 44) return 3;
    return 0;
}
