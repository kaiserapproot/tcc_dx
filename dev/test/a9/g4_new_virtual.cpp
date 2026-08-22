// G4: a heap object has no declaration site, so `new` has to write the
// vptr itself - otherwise a virtual call through the returned pointer
// dispatches on garbage.  Covers the CPPUnit shape
// `(TestListener*)new Logger(stdout)`: allocate a derived object and use
// it through a base pointer.
// NOTE: the destructors here are deliberately NON-virtual - `virtual ~T()`
// is BUG-24 / G6.  Each object is therefore deleted through its own static
// type, which is all G4 promises.
int g_dtor_calls;
class Base {
public:
    int b;
    Base() { b = 1; }
    virtual int who() { return 1; }
    ~Base() { g_dtor_calls = g_dtor_calls + 1; }
};
class Derived : public Base {
public:
    int d;
    Derived() { d = 2; }
    virtual int who() { return 2; }
    ~Derived() { g_dtor_calls = g_dtor_calls + 10; }
};
int main()
{
    Base* p;
    Derived* q;

    g_dtor_calls = 0;
    p = new Base();
    if (p->who() != 1 || p->b != 1)
        return 1;
    delete p;
    if (g_dtor_calls != 1)
        return 2;

    q = new Derived();
    if (q->who() != 2)
        return 3;
    if (q->b != 1 || q->d != 2)
        return 4;
    p = (Base*)q;
    if (p->who() != 2)          /* dynamic dispatch through the base ptr */
        return 5;
    delete q;
    /* 1 (Base above) + 10 (~Derived) + 1 (~Base, chained) */
    if (g_dtor_calls != 12)
        return 6;
    return 0;
}
