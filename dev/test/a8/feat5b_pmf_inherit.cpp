class Base {
public:
    int v;
    void reset();
};

class Derived : public Base {
public:
    int extra;
};

void Base::reset() { v = 0; }

int main() {
    void (Base::*pmf)() = &Base::reset;
    Derived d;
    d.v = 5;
    (d.*pmf)();
    return d.v;
}
