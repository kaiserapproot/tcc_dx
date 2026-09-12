struct M {
    static int count;
    int ordinal;

    M()
    {
        ordinal = ++count;
    }
};

int M::count;

int main()
{
    M a[4];

    if (M::count != 4)
        return 1;
    if (a[0].ordinal != 1)
        return 2;
    if (a[1].ordinal != 2)
        return 3;
    if (a[2].ordinal != 3)
        return 4;
    if (a[3].ordinal != 4)
        return 5;
    return 0;
}
