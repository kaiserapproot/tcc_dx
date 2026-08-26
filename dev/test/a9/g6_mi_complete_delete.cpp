// G6 (plan-mandated, most important): delete through a NON-primary base
// pointer of a multiple-inheritance object.  b != d (pointer adjustment),
// so free(b) would corrupt the heap; the vtable's offset-to-top must
// recover the complete object.  The digit-shift state proves the dtor
// order D body -> ~B -> ~A (bases in reverse declaration order) - 123 is
// the only passing order.
int state;
struct A {
    int a;
    virtual ~A() { state = state * 10 + 3; }
};
struct B {
    int b;
    virtual ~B() { state = state * 10 + 2; }
};
struct D : A, B {
    int d;
    ~D() { state = state * 10 + 1; }
};
int main()
{
    D* d;
    B* b;
    A* a;

    state = 0;
    d = new D();
    b = (B*)d;
    if ((void*)b == (void*)d)   /* the adjustment must actually happen */
        return 9;
    delete b;                   /* complete-object free via offset-to-top */
    if (state != 123)
        return 1;

    /* primary-base pointer: adjustment is 0 but the same path runs */
    state = 0;
    d = new D();
    a = (A*)d;
    delete a;
    if (state != 123)
        return 2;
    return 0;
}
