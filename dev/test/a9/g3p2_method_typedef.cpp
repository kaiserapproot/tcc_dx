// G3 P2: class typedefs resolve inside member function bodies - both
// the deferred in-class inline body and the out-of-class definition.
class C {
public:
    typedef int T;
    int f();
    int g()
    {
        T v;
        v = 3;
        return v;
    }
};
int C::f()
{
    T v;
    v = 4;
    return v;
}
int main()
{
    C c;
    if (c.f() != 4)
        return 1;
    if (c.g() != 3)
        return 2;
    return 0;
}
