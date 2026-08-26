// G3 negative (rev.3 Blocker 2-C): a non-type local hides the class
// typedef - the lookup must NOT fall back to C::T.
class C {
public:
    typedef int T;
    int f();
};
int C::f()
{
    int T = 0;
    T x;
    return T;
}
int main()
{
    return 0;
}
