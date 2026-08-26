// G-OP (plan rev.7 High 2): operator->() returns a pointer to an object
// UNRELATED to the proxy itself, so an implementation that wrongly went
// through "this" would be caught here, for reads and writes.
struct P {
    int v;
};
struct Proxy {
    P* target;
    P* operator->() { return target; }
};
int main()
{
    P a;
    P b;
    Proxy x;
    a.v = 11;
    b.v = 77;
    x.target = &b;
    if (x->v != 77)
        return 1;
    x->v = 88;
    if (b.v != 88)
        return 2;
    if (a.v != 11)
        return 3;
    return 0;
}
