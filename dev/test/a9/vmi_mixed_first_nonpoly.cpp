// Virtual MI (Phase 2): non-polymorphic FIRST base + polymorphic second
// base.  The derived class gets its own vptr at offset 0 (all bases shift),
// the B subobject keeps its own vptr, and upcasts to both bases adjust the
// pointer past the inserted vptr.
class A {
public:
    int a;
    int getA() { return a; }
};
class B {
public:
    int b;
    virtual int fb() { return b; }
};
class D : public A, public B {
public:
    int d;
    virtual int fb() { return 1000 + b; }
    virtual int fd() { return d; }
};

int main()
{
    D o;
    A *pa;
    B *pb;
    o.a = 1; o.b = 2; o.d = 3;
    if (o.getA() != 1) return 1;     // non-poly base method, this += off(A)
    if (o.fb() != 1002) return 2;    // own override via own vtable
    if (o.fd() != 3) return 3;
    pa = (A *)&o;
    if (pa->getA() != 1) return 4;   // upcast adjusts past the own vptr
    pb = (B *)&o;
    if (pb->fb() != 1002) return 5;  // secondary vtable -> thunk
    if (pb->b != 2) return 6;
    return 0;
}
