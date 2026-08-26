// G3 negative (rev.3 Blocker 2-B): bases naming DIFFERENT types for the
// same typedef name must be an ambiguity error, never first-base-wins.
struct A {
    typedef int T;
};
struct B {
    typedef char T;
};
struct D : A, B {
    T x;
};
int main()
{
    return 0;
}
