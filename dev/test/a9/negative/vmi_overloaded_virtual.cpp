// NEGATIVE test: must FAIL to compile.
// A secondary (non-primary) base's vtable is built by matching overrides
// BY NAME, so an overloaded virtual cannot be resolved: both slots would bind
// to whichever override is found first.  Before the guard this compiled
// cleanly and produced a silent miscompile - `pb->f(1)` and `pb->f(2.0)` both
// dispatched to the same implementation.
// Rejecting it is the conservative behaviour until slot matching becomes
// signature-aware.  The primary base is unaffected (offset 0, shared vtable).
struct A {
    int a;
    virtual int fa() { return 1; }
};

struct B {
    int b;
    virtual int f(int x)    { return x + 100; }
    virtual int f(double x) { return (int)x + 200; }
};

struct D : public A, public B {
    int d;
    virtual int f(int x)    { return x + 1000; }
    virtual int f(double x) { return (int)x + 2000; }
};

int main(void)
{
    D dd;
    B *pb;

    dd.a = 0;
    dd.b = 0;
    dd.d = 0;
    pb = &dd;
    return pb->f(1) + pb->f(2.0);
}
