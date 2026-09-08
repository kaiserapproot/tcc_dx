// N7-00 case D: member without default ctor — implicit default ctor ill-formed.
struct M {
    M(int x) { (void)x; }
};
struct A {
    M m;
};
int main(void)
{
    A a;
    (void)a;
    return 0;
}
