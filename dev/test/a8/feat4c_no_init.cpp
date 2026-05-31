class Foo {
public:
    int a;
    Foo(int x);
};

Foo::Foo(int x) {
    a = x;
}

int main() {
    Foo f(5);
    return f.a - 5;
}
