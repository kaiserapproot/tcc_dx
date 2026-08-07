// Virtual MI (Phase 2): an override reached through the non-primary base
// pointer must mutate the ORIGINAL object (thunk `this` identity), and its
// body must see the members of every base of the derived object.  The
// object is larger than 8 bytes, so this also guards the BUG-15/16 family
// (by-value `this` copies) on the virtual-MI path.
class A {
public:
    int a;
    virtual int fa() { return a; }
};
class B {
public:
    int b;
    virtual int poke(int v) { b = v; return 0; }
};
class D : public A, public B {
public:
    int d;
    virtual int poke(int v) { a = v; b = v + 1; d = v + 2; return a + b + d; }
};

int main()
{
    D o;
    B *pb;
    int r;
    o.a = 0; o.b = 0; o.d = 0;
    pb = &o;
    r = pb->poke(5);                  // thunk -> D::poke(this = &o)
    if (r != 5 + 6 + 7) return 1;
    if (o.a != 5) return 2;           // wrote the real object, not a copy
    if (o.b != 6) return 3;
    if (o.d != 7) return 4;
    return 0;
}
