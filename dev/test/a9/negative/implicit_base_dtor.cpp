// An implicit derived destructor must not skip a non-trivial base
// until the compiler can emit the base's destructor call.
struct B {
    ~B() { }
};

struct D : B {
};

int main()
{
    D d;
    return 0;
}
