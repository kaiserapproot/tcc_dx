// N7-00 case E: base without default ctor — implicit default ctor ill-formed.
struct B {
    B(int x) { (void)x; }
};
struct D : B {
};
int main(void)
{
    D d;
    (void)d;
    return 0;
}
