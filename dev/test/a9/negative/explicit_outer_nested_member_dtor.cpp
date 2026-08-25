// An explicit outer dtor must not skip a nested implicit member dtor.
struct N {
    ~N() { }
};

struct M {
    N n;
};

struct X {
    M m;
    ~X() { }
};

int main()
{
    X x;
    return 0;
}
