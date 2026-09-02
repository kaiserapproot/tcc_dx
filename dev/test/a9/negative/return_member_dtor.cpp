struct M {
    ~M() {}
};

struct H {
    M m;
};

H make_member(H& source)
{
    return source;
}

int main()
{
    return 0;
}
