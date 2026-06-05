class Foo { public: int v; Foo(int x) : v(x) {} };
int main() {
    Foo a(1), b(2), c(3);
    return (a.v + b.v + c.v) - 6;
}
