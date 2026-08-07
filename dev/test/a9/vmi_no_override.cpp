// Virtual MI (Phase 2): derived does not override anything - a virtual
// member of the NON-primary base found on the derived object must dispatch
// through the subobject's vptr (base slot numbering), adjusting `this`.
class A {
public:
    int a;
    virtual int fa() { return 100 + a; }
};
class B {
public:
    int b;
    virtual int fb() { return 200 + b; }
};
class D : public A, public B {
public:
    int d;
};

int main()
{
    D o;
    B *pb;
    o.a = 1; o.b = 2; o.d = 3;
    if (o.fa() != 101) return 1;   // primary base virtual, offset 0
    if (o.fb() != 202) return 2;   // non-primary: this += off(B), B's vptr
    pb = &o;
    if (pb->fb() != 202) return 3; // through the adjusted B*
    return 0;
}
