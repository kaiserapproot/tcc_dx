// BUG-31: an out-of-class STATIC DATA member definition must not leak its
// class into the next function definition.  It used to, so `probe` was
// registered as a member of S, gained an implicit `this` parameter, and
// the caller's argument landed in the `this` slot - a silent miscompile
// with no overloads or templates involved.
class S {
public:
    static const int npos;
    int m;
};
const int S::npos = 99;
void probe(S& s)
{
    s.m = 7;
}
int add3(int a, int b, int c)
{
    return a + b + c;
}
int main()
{
    S s;
    s.m = 0;
    probe(s);
    if (s.m != 7)
        return 1;
    if (S::npos != 99)
        return 2;
    // a free function taking several args after the same pattern
    if (add3(1, 2, 3) != 6)
        return 3;
    return 0;
}
