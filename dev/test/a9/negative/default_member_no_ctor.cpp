// A user-declared member constructor with no viable zero-argument overload
// must reject an enclosing constructor that omits the member initializer.
struct M {
    M(int value) { }
};

struct X {
    M m;
    X() { }
};

int main()
{
    X x;
    return 0;
}
