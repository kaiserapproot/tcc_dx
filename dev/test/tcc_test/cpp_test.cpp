#include <stdio.h>
class Foo {
public:
    int a;
    Foo() : a(5) {}
};
class Bar {
public:
    Foo f;
    Bar() : f() {
        f.a = 10;
    }
};
int main() {
    Bar b;
    printf("%d\n", b.f.a);
    return 0;
}