struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

struct X {
    M m;
};

int main()
{
    X a[3];

    if (M::count != 3)
        return 1;
    return 0;
}
