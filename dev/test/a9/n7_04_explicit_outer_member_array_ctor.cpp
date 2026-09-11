struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

struct X {
    M m[3];

    X()
    {
    }
};

int main()
{
    X x;
    return M::count == 3 ? 0 : 1;
}
