// An explicit outer ctor must not skip a nested implicit member ctor.
struct N {
    N() { }
};

struct M {
    N n;
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
