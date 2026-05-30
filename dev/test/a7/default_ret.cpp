class Foo {
public:
    int bar(int x = 10);
};
int Foo::bar(int x) { return x; }
int main() { Foo f; return f.bar(10) == 10 ? 0 : 3; }