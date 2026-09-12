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
    static M a[4];

    if (a[0].ordinal != 1)
        abort();
    if (a[1].ordinal != 2)
        abort();
    if (a[2].ordinal != 3)
        abort();
    if (a[3].ordinal != 4)
        abort();
}

int main()
{
    touch();
    if (M::count != 4)
        return 1;
    touch();
    if (M::count != 4)
        return 2;
    touch();
    if (M::count != 4)
        return 3;
    return 0;
}
