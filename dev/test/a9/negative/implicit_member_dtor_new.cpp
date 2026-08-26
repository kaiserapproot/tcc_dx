// Heap destruction must apply the same fail-closed policy as local
// destruction when the outer class has no destructor of its own.
struct M {
    ~M() { }
};

struct X {
    M m;
};

int main()
{
    X *p = new X;
    delete p;
    return 0;
}
