struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

void touch()
{
    static M a[4];
    (void)a;
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
