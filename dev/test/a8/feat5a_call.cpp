class Foo {
public:
    virtual int foo() { return 42; }
};
int main() {
    Foo f;
    return f.foo() - 42;
}
