// Virtual MI (Phase 2): global object - every vptr is wired statically via
// data relocations, including the secondary (non-primary base) one.
class A {
public:
    int a;
    virtual int fa() { return 10 + a; }
};
class B {
public:
    int b;
    virtual int fb() { return 20 + b; }
};
class D : public A, public B {
public:
    virtual int fb() { return 30 + b; }
};

D g;

int main()
{
    B *pb;
    g.a = 1; g.b = 2;
    if (g.fa() != 11) return 1;      // inherited primary-base virtual
    if (g.fb() != 32) return 2;      // own override
    pb = &g;
    if (pb->fb() != 32) return 3;    // static secondary vptr -> thunk
    return 0;
}
