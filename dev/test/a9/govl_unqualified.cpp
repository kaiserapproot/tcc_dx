// BUG-30 / G-OVL: the same forward reference through the unqualified
// (BUG-22 implicit `this`) form and the explicit `this->` form, both from
// a member body compiled before the callee's definition.
class S {
public:
    int m;
    int f(int a);
    int f(int a, int b);
    int viaPlain();
    int viaThis();
};
int S::viaPlain()
{
    return f(1, 2);
}
int S::viaThis()
{
    return this->f(9);
}
int S::f(int a) { m = 10 + a; return m; }
int S::f(int a, int b) { m = a + b; return m; }
int main()
{
    S s;
    s.m = 0;
    if (s.viaPlain() != 3 || s.m != 3)
        return 1;
    if (s.viaThis() != 19 || s.m != 19)
        return 2;
    return 0;
}
