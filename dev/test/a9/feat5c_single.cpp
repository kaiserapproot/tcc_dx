// FEAT-5C: virtual PMF on a single class with no derived override still
// dispatches correctly through the vtable (regression guard for the simple
// case, and that non-virtual PMF is unaffected).
class Foo {
public:
    Foo() {}
    virtual int val() { return 42; }
    int plain() { return 7; }          // non-virtual, for contrast
};

int main()
{
    int (Foo::*vp)() = &Foo::val;      // virtual PMF
    int (Foo::*np)() = &Foo::plain;    // non-virtual PMF (FEAT-5B)
    Foo f;
    return (f.*vp)() * 10 + (f.*np)() - 427;   // 42*10 + 7 - 427 = 0
}
