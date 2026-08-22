// G5: pure virtual declarations, abstract classes, and dispatch through a
// base pointer to an override.  The abstract-ness check walks the FINAL
// vtable, so it inherits: B below overrides nothing and stays abstract,
// C overrides and becomes concrete.
class A {
public:
    int tag;
    virtual int f() = 0;
    virtual int g() { return 100; }
};
class B : public A {
public:
    /* no override of f() -> still abstract */
    virtual int g() { return 200; }
};
class C : public B {
public:
    virtual int f() { return 7; }
};
class D : public A {
public:
    virtual int f() { return 9; }
    virtual int g() { return 300; }
};
int main()
{
    C c;
    D d;
    A* p;
    B* q;

    c.tag = 1;
    if (c.f() != 7 || c.g() != 200)
        return 1;
    if (d.f() != 9 || d.g() != 300)
        return 2;

    p = (A*)&c;                 /* dispatch through the abstract base */
    if (p->f() != 7 || p->g() != 200)
        return 3;
    p = (A*)&d;
    if (p->f() != 9 || p->g() != 300)
        return 4;

    q = (B*)&c;
    if (q->f() != 7)
        return 5;

    /* heap object of a concrete derived class, used as the abstract base */
    p = (A*)new C();
    if (p->f() != 7 || p->g() != 200)
        return 6;
    delete (C*)p;
    return 0;
}
