// Explicit qualified base-member call from inside a member body - the
// TestSetup.cpp:51 shape (`TestDecorator::run(m_result);`).  A call
// written `Base::method(args)` must bind the NAMED class's
// implementation directly (no vtable dispatch) with `this` adjusted to
// that base subobject; without the fix the postfix `::` handler only
// knew static members and died with "static member not found".
struct Base {
    int v;
    virtual int run(int x) { return v + x; }
    int plain(int x) { return v * x; }
};
struct Deco : Base {
    // qualified call to the virtual base method: must NOT re-dispatch
    // (re-dispatching from Setup would recurse into Setup::run forever)
    virtual int run(int x) { return Base::run(x) * 2; }
};
struct Setup : Deco {
    virtual int run(int x) { return Deco::run(x) + 1; }
    int callPlain(int x) { return Base::plain(x); }   // non-virtual target
};
int main()
{
    Setup s;
    Base *b = &s;

    s.v = 10;
    // virtual chain: Setup::run -> Deco::run -> Base::run = (10+5)*2+1
    if (b->run(5) != 31)
        return 1;
    // qualified call to a non-virtual base method
    if (s.callPlain(3) != 30)
        return 2;
    return 0;
}
