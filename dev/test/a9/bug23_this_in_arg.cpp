// BUG-23: a member call whose ARGUMENT expression also touches `this`.
// `this` is stashed off the vstack in cpp_member_this while the arguments are
// parsed, so the register allocator did not know it was still live.  Evaluating
// an argument that reads a member reloads `this` and reused that register, so
// the injected `this` became garbage - an access violation at run time with no
// diagnostic.  `this->bump(this->v + 1)` failed identically, so it was never
// specific to the unqualified call form.
struct Counter {
    int v;
    int bump(int x) { v = x; return 1; }

    int from_member()      { return bump(v + 1); }          // the reported case
    int from_member_this() { return this->bump(this->v + 1); }
    int two_args(int a, int b) { v = a + b; return v; }
    int nested()           { return two_args(v, v * 2); }   // both args use this
    int chained()          { return from_member() + bump(v + 1); }
};

// same shape with a virtual member: dispatch goes through the vtable while the
// argument is being evaluated
struct VBase {
    int v;
    virtual int set(int x) { v = x; return 1; }
    int go()               { return set(v + 5); }
};

struct VDer : public VBase {
    virtual int set(int x) { v = x * 2; return 2; }
};

// and with a base-class member reached from the derived class
struct Helper {
    int hv;
    int put(int x) { hv = x; return hv; }
};

struct UsesHelper : public Helper {
    int use() { return put(hv + 7); }
};

int main(void)
{
    Counter c;
    VBase b;
    VDer d;
    VBase *bp;
    UsesHelper u;

    c.v = 5;
    if (c.from_member() != 1) return 1;
    if (c.v != 6)             return 1;

    c.v = 10;
    if (c.from_member_this() != 1) return 1;
    if (c.v != 11)                 return 1;

    c.v = 3;
    if (c.nested() != 9) return 1;      /* 3 + 6 */
    if (c.v != 9)        return 1;

    c.v = 1;
    /* from_member(): v=2 returns 1, then bump(v+1): v=3 returns 1 */
    if (c.chained() != 2) return 1;
    if (c.v != 3)         return 1;

    b.v = 1;
    if (b.go() != 1) return 1;
    if (b.v != 6)    return 1;          /* VBase::set(1+5) */

    d.v = 1;
    bp = &d;
    if (bp->go() != 2) return 1;
    if (d.v != 12)     return 1;        /* VDer::set(1+5) -> 6*2 */

    u.hv = 2;
    if (u.use() != 9) return 1;
    if (u.hv != 9)    return 1;
    return 0;
}
