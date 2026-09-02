struct M {
    ~M() {}
};

struct H {
    M items[2][3][4];
};

H make_multidim_3d(H& source)
{
    return source;
}

int main()
{
    return 0;
}
