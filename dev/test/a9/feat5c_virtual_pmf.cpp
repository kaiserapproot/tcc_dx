// FEAT-5C: pointer-to-member of a VIRTUAL function.  Taking &Base::who and
// invoking it through an object dispatches on the object's dynamic type via
// the vtable (not a fixed address).  Covers a base object, a derived object,
// and true polymorphism through a base pointer.  (Requires BUG-14's
// class-qualified member link names so the base slot is not clobbered.)
class Base {
public:
    Base() {}
    virtual int who() { return 1; }
};
class Derived : public Base {
public:
    Derived() {}
    virtual int who() { return 2; }
};

int main()
{
    int (Base::*pmf)() = &Base::who;
    Base b;
    Derived d;
    int rb = (b.*pmf)();          // Base object   -> Base::who    = 1
    int rd = (d.*pmf)();          // Derived object -> Derived::who = 2
    Base *p = (Base *)&d;
    int rp = (p->*pmf)();         // Base* to Derived -> Derived::who = 2
    return rb * 100 + rd * 10 + rp - 122;   // 100 + 20 + 2 - 122 = 0
}
