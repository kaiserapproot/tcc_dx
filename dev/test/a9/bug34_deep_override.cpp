// BUG-34: C overrides a virtual declared in its GRANDPARENT that the
// intermediate class never redeclared.  The override used to take a new
// vtable slot, so a call through a base pointer still reached the
// grandparent's implementation - silent miscompile, no diagnostic.
class A {
public:
    int a;
    virtual int f() { return 1; }
    virtual int g() { return 10; }
    virtual int h() { return 100; }
};
class B : public A {
public:
    int b;
    virtual int g() { return 20; }
};
class C : public B {
public:
    int c;
    virtual int f() { return 7; }       /* skips a level */
    virtual int h() { return 700; }     /* skips a level */
};
class D : public C {
public:
    virtual int g() { return 40; }      /* overrides B's override */
};
int main()
{
    C c;
    D d;
    A* p;
    B* q;

    c.a = 1;
    c.b = 2;
    c.c = 3;
    if (c.f() != 7 || c.g() != 20 || c.h() != 700)
        return 1;

    p = (A*)&c;
    if (p->f() != 7)
        return 2;
    if (p->g() != 20)
        return 3;
    if (p->h() != 700)
        return 4;

    q = (B*)&c;
    if (q->f() != 7 || q->h() != 700)
        return 5;

    p = (A*)&d;
    if (p->f() != 7 || p->g() != 40 || p->h() != 700)
        return 6;

    /* data members must still be where they were */
    if (c.a != 1 || c.b != 2 || c.c != 3)
        return 7;
    return 0;
}
