struct M {
    ~M() {}
};

struct H {
    M items[2];
};

H make_array(H& source)
{
    return source;
}

int main()
{
    return 0;
}
