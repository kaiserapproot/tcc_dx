struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

void f()
{
    static M a[4];
    (void)a;
}

int main()
{
    f();
    if (M::count != 4)
        return 1;
    f();
    if (M::count != 4)
        return 2;
    f();
    if (M::count != 4)
        return 3;
    return 0;
}
