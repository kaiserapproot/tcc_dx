class Foo {
public:
    Foo() {}
    virtual int foo() { return 7; }
};

int main() {
    Foo f;
    Foo *p;

    p = &f;
    return p->foo() - 7;
}
