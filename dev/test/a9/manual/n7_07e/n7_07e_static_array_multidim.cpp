struct M {
    static int count;
    int ordinal;

    M()
    {
        ordinal = ++count;
    }
};

int M::count;

static void touch()
{
    static M a[2][3];

    if (M::count != 6)
        abort();
    if (a[0][0].ordinal != 1)
        abort();
    if (a[0][1].ordinal != 2)
        abort();
    if (a[0][2].ordinal != 3)
        abort();
    if (a[1][0].ordinal != 4)
        abort();
    if (a[1][1].ordinal != 5)
        abort();
    if (a[1][2].ordinal != 6)
        abort();
}

int main()
{
    touch();
    if (M::count != 6)
        return 1;
    touch();
    if (M::count != 6)
        return 2;
    return 0;
}
