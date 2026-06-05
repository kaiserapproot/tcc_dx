static int g_dtor;

class Foo {
public:
    int a;
    Foo(int x) : a(x) {}
    ~Foo() { g_dtor++; }
};

int main() {
    { Foo f(1); }
    return g_dtor - 1;
}
