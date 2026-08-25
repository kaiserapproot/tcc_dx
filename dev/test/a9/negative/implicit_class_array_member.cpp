// Class-type member arrays need one default constructor call per element;
// reject them until the implicit path has an element walker.
struct M {
    M() { }
};

struct X {
    M m[2];
};

int main()
{
    X x;
    return 0;
}
