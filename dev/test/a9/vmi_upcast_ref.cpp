// Virtual MI (Phase 2): reference upcasts (parameter and local) to the
// non-primary polymorphic base, with virtual dispatch through the reference.
class A {
public:
    int a;
    virtual int fa() { return a; }
};
class B {
public:
    int b;
    virtual int fb() { return b; }
};
class D : public A, public B {
public:
    virtual int fb() { return 300 + b; }
};

int callB(B &r) { return r.fb(); }    // reference-parameter upcast

int main()
{
    D o;
    o.a = 1; o.b = 2;
    if (callB(o) != 302) return 1;    // param ref -> B subobject -> thunk
    {
        B &lr = o;                    // local reference upcast
        if (lr.fb() != 302) return 2;
        if (lr.b != 2) return 3;
    }
    return 0;
}
