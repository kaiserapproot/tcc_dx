// An explicit outer ctor must not accept an unconstructed class member array.
struct M {
    M() { }
};

struct X {
    M m[2];
    X() { }
};

int main()
{
    X x;
    return 0;
}
