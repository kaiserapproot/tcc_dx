// C++98 implicit copy construction through `new` - the TestResult.cpp:64
// shape `m_failures.push_back(new TestFailure(*failure));` where
// TestFailure declares only TestFailure(Test*, ...) and the copy ctor is
// the IMPLICIT one.  Covers: class whose only user ctor is unrelated,
// ctor-less POD class, and a polymorphic class (the copied vptr must
// dispatch correctly).
struct F {
    int *t;
    long line;
    F(int *tt, long l) : t(tt), line(l) {}
};
struct P {
    int x;
    int y;
};
struct V {
    int v;
    V(int a) : v(a) {}
    virtual int get() { return v + 1; }
};
int main()
{
    int q;
    F f(&q, 7);
    const F *fp = &f;
    F *fc = new F(*fp);         // user ctor takes (int*,long) - not viable
    if (fc->t != &q || fc->line != 7)
        return 1;
    P p;
    p.x = 3;
    p.y = 4;
    P *pc = new P(p);           // ctor-less class
    if (pc->x != 3 || pc->y != 4)
        return 2;
    V ov(5);
    V *vc = new V(ov);          // vptr comes from the source copy
    if (vc->get() != 6)
        return 3;
    delete fc;
    delete pc;
    return 0;
}
