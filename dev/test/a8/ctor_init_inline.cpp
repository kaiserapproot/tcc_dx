class Foo {
public:
    int a;
    Foo(int x) : a(x) {}
};
int main() {
    Foo f;
    f.Foo(5);
    return f.a - 5;
}
