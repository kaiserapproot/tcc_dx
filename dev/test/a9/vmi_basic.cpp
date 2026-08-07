// Virtual MI (Phase 2): two polymorphic bases, derived overrides both.
// Dispatch through the derived object, through A* (primary base, shared
// vptr at offset 0) and through B* (secondary vtable + this-adjust thunk).
class A {
public:
    int a;
    virtual int fa() { return 1; }
};
class B {
public:
    int b;
    virtual int fb() { return 2; }
};
class D : public A, public B {
public:
    int d;
    virtual int fa() { return 10 + a; }
    virtual int fb() { return 20 + b; }
};

int main()
{
    D o;
    A *pa;
    B *pb;
    o.a = 1; o.b = 2; o.d = 3;
    if (o.fa() != 11) return 1;      // direct call -> override
    if (o.fb() != 22) return 2;      // direct call -> override
    pa = &o;
    if (pa->fa() != 11) return 3;    // primary base pointer -> override
    pb = &o;
    if (pb->fb() != 22) return 4;    // secondary vtable -> thunk -> override
    if (pb->b != 2) return 5;        // data through the adjusted B*
    return 0;
}
