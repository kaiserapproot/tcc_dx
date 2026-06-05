class Foo {
public:
    Foo() {}
    virtual int foo() { return 42; }
};

int main() {
    Foo f;
    return f.foo() - 42;
}
