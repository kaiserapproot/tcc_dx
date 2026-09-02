struct Leaf {
    ~Leaf() {}
};

struct M {
    Leaf leaf;
};

struct H {
    M items[2][3];
};

H make_transitive(H& source)
{
    return source;
}

int main()
{
    return 0;
}
