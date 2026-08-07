// Virtual MI (Phase 2): constructors on a polymorphic-MI class - explicit
// mem-init for both bases plus a derived ctor body, then virtual dispatch
// through the derived object and the non-primary base pointer.
class A {
public:
    int a;
    A() { a = 1; }
    virtual int fa() { return a; }
};
class B {
public:
    int b;
    B() { b = 2; }
    virtual int fb() { return b; }
};
class D : public A, public B {
public:
    int d;
    D() : A(), B() { d = 3; }
    virtual int fb() { return b + d; }
};

int main()
{
    D o;
    B *pb;
    if (o.fa() != 1) return 1;
    if (o.fb() != 5) return 2;
    pb = &o;
    if (pb->fb() != 5) return 3;
    return 0;
}
