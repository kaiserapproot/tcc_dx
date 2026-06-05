class Foo { public: int a; Foo(int x) : a(x) {} };
int main() {
    Foo f(5);
    return f.a - 5;
}
