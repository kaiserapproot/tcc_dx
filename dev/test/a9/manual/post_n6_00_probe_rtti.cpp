struct Base { virtual int f() { return 1; } };
struct Derived : Base { int f() { return 2; } };
int main(void)
{
    Derived d;
    Base *p = &d;
    Derived *q = dynamic_cast<Derived*>(p);
    return q ? 0 : 1;
}
