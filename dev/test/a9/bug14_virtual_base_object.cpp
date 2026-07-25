// BUG-14: when a derived class overrides a base virtual with the SAME name,
// the base and the override shared one link name.  external_sym merged them,
// so the base class's vtable slot was relocated against the override's body:
// a Base object then dispatched to Derived's method.  With class-qualified
// member link names the base slot points at the base impl again.
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
    Base b;
    Derived d;
    // b is a Base object: must call Base::who (1), even though Derived
    // overrides who().  d must call Derived::who (2).
    int rb = b.who();
    int rd = d.who();
    // and true polymorphism through a base pointer still reaches Derived:
    Base *p = (Base *)&d;
    int rp = p->who();               // Derived::who via vtable -> 2
    return rb * 100 + rd * 10 + rp - 122;   // 100 + 20 + 2 - 122 = 0
}
