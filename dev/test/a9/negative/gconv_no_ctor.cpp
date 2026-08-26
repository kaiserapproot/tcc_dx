// G-CONV negative: an incompatible assignment to a class WITHOUT a
// matching converting constructor must keep failing - the hook may not
// swallow the diagnostic.
struct P {
    int v;
    P(int a) : v(a) {}
};
int main()
{
    P p(1);
    p = "nope";
    return p.v;
}
