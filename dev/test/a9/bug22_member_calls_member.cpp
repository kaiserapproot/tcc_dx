// BUG-22: an unqualified call to another member of the same class must pass
// `this`.  The name binds to the hoisted global that holds the method body,
// whose first parameter IS `this`, so the call went out one argument short and
// crashed at run time (access violation) rather than failing to compile.
// amateras cross.h relies on this everywhere - window_t::init() is just
// `return init_common(...)`.
//
// Two neighbouring gaps are deliberately NOT exercised here; both predate this
// fix and both still reproduce through the explicit `this->` form, so they are
// not regressions of it:
//   - a VIRTUAL destructor (`virtual ~Base()`) fails to parse.
//   - an argument expression that itself dereferences `this`
//     (`bump(v + 1)`, and equally `this->bump(this->v + 1)`) crashes: `this`
//     is captured off the vstack into cpp_member_this, so evaluating the
//     argument can clobber it (see the BUG-18 note).  Arguments that are
//     constants or locals are fine, which is what this test uses.
struct Counter {
    int v;
    int bump(int x) { v = x; return 1; }
    int go()        { return bump(42); }        // member -> member
    int twice()     { return go() + go(); }     // two member calls in one expr
};

// arguments of several types, and the return value actually used
struct Math {
    int   acc;
    float facc;
    int   addi(int a, int b)     { acc = a + b; return acc; }
    float addf(float a, float b) { facc = a + b; return facc; }
    int   both()                 { return addi(2, 3) + (int)addf(0.5f, 1.5f); }
};

// virtual dispatch must still go through the vtable when called unqualified
struct Base {
    int tag;
    virtual int who() { return 1; }
    int         ask() { return who(); }   // unqualified virtual call
};

struct Derived : public Base {
    virtual int who() { return 2; }
};

// a member inherited from a base, called unqualified from the derived class
struct BaseHelper {
    int hv;
    int helper(int x) { hv = x * 2; return hv; }
};

struct UsesBase : public BaseHelper {
    int use() { return helper(21); }      // resolves into the base subobject
};

int main(void)
{
    Counter c;
    Math m;
    Base b;
    Derived d;
    Base *bp;
    UsesBase u;

    c.v = 0;
    if (c.go() != 1) return 1;
    if (c.v != 42)   return 1;

    c.v = 0;
    if (c.twice() != 2) return 1;
    if (c.v != 42)      return 1;

    if (m.both() != 7) return 1;       /* 5 + (int)2.0f */
    if (m.acc != 5)    return 1;
    if (m.facc != 2.0f) return 1;

    b.tag = 0;
    if (b.ask() != 1) return 1;        /* Base::who */
    d.tag = 0;
    if (d.ask() != 2) return 1;        /* Derived::who through the vtable */
    bp = &d;
    if (bp->ask() != 2) return 1;

    if (u.use() != 42) return 1;
    if (u.hv != 42)    return 1;
    return 0;
}
