// An implicitly declared derived default constructor must validate its
// base subobjects before accepting the object declaration.
struct B {
    B(int value) { }
};

struct D : B {
};

int main()
{
    D d;
    return 0;
}
