// An implicitly declared outer default constructor must validate its
// class-type members even when the outer class declares no constructor.
struct M {
    M(int value) { }
};

struct X {
    M m;
};

int main()
{
    X x;
    return 0;
}
