class Foo {
public:
    int a;
    int b;
    Foo(int x, int y) : a(x), b(y) {}
};
int main() {
    Foo f;
    f.Foo(3, 4);
    return f.a + f.b - 7;
}
