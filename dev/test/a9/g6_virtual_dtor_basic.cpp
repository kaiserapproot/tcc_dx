// G6 / BUG-24: virtual destructor, single inheritance.  `delete base_ptr`
// must run the DERIVED dtor first, then the base dtor (digit-shift state
// so only the exact order passes).  Also checks: parsing `virtual ~B()`,
// implicit virtual on the derived dtor (no keyword written), sizeof gains
// a vptr, and an explicit virtual p->~B() call dispatching dynamically.
int state;
struct B {
    int b;
    virtual ~B() { state = state * 10 + 2; }
};
struct D : B {
    int d;
    ~D() { state = state * 10 + 1; }        /* implicitly virtual */
};
int main()
{
    B* p;
    D* q;

    if (sizeof(B) != sizeof(void*) + 8)     /* vptr + int (padded) */
        return 10;

    state = 0;
    p = (B*)new D();
    delete p;                                /* ~D then ~B */
    if (state != 12)
        return 1;

    state = 0;
    q = new D();
    delete q;                                /* same order via D* */
    if (state != 12)
        return 2;

    state = 0;
    p = new B();
    delete p;
    if (state != 2)
        return 3;

    /* explicit dtor call through a base pointer: dynamic dispatch,
       no free involved */
    {
        D d2;
        d2.b = 0;
        d2.d = 0;
        state = 0;
        p = (B*)&d2;
        p->~B();
        if (state != 12)
            return 4;
        state = 0;      /* the scope-exit dtor will run again; ignore it */
    }
    return 0;
}
