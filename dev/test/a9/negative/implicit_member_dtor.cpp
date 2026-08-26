// An implicit outer destructor must not skip a non-trivial member
// until the compiler can emit the member's destructor call.
struct M {
    ~M() { }
};

struct X {
    M m;
};

int main()
{
    X x;
    return 0;
}
