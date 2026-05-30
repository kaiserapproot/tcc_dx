class Foo {
public:
    static int bar(int x = 10);
};
int Foo::bar(int x) { return x; }
int main() { return Foo::bar() == 10 ? 0 : 2; }