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
    M a[2][3];

    if (M::count != 6)
        return 1;
    if (a[0][0].ordinal != 1)
        return 2;
    if (a[0][1].ordinal != 2)
        return 3;
    if (a[0][2].ordinal != 3)
        return 4;
    if (a[1][0].ordinal != 4)
        return 5;
    if (a[1][1].ordinal != 5)
        return 6;
    if (a[1][2].ordinal != 6)
        return 7;
    return 0;
}
