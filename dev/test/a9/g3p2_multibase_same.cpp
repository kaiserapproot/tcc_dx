// G3 P2 (rev.4 Blocker 1): two bases whose typedefs NAME the same type
// must merge without a false ambiguity error.
struct A {
    typedef int T;
};
struct B {
    typedef int T;
};
struct D : A, B {
    T x;
};
int main()
{
    D d;
    d.x = 5;
    if (sizeof(d.x) != sizeof(int))
        return 1;
    return d.x == 5 ? 0 : 2;
}
