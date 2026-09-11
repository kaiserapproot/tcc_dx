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
