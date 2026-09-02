struct B {
    ~B() {}
};

struct D : B {
};

D make_base(D& source)
{
    return source;
}

int main()
{
    return 0;
}
