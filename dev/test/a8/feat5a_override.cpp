class Base {
public:
    Base() {}
    virtual int foo() { return 1; }
};

class Derived : public Base {
public:
    Derived() {}
    virtual int foo() { return 2; }
};

int main() {
    Derived d;
    Base *p;

    p = (Base *)&d;
    return p->foo() - 2;
}
