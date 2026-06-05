class Foo {
public:
    virtual int foo() { return 42; }
};
Foo g;
int main() {
    return g.foo() - 42;
}
