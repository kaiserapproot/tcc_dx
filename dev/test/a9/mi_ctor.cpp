// MI Phase 1: a derived ctor's mem-initializer list constructs each base
// subobject.  The non-first base's ctor runs on its own subobject (this
// adjusted), so its member writes land in the right place.
class A {
public:
    int a;
    A(int x) { a = x; }
};
class B {
public:
    int b, c;
    B(int x, int y) { b = x; c = y; }    // non-first base, 2 members
};
class D : public A, public B {
public:
    int d;
    D(int x) : A(x), B(x * 2, x * 3) { d = x * 4; }
};

int main()
{
    D o(5);
    // A(5) -> a=5 ; B(10,15) -> b=10,c=15 ; d=20
    return o.a * 1 + o.b * 10 + o.c * 100 + o.d
           - (5 + 100 + 1500 + 20);      // 5+100+1500+20 = 1625
}
