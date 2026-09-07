struct A { virtual int f() { return 1; } };
struct B : virtual A { int f() { return 2; } };
int main(void)
{
    B b;
    return b.f();
}
