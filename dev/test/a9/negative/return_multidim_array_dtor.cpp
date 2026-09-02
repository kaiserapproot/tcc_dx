struct M {
    ~M() {}
};

struct H {
    M items[2][3];
};

H make_multidim(H& source)
{
    return source;
}

int main()
{
    return 0;
}
