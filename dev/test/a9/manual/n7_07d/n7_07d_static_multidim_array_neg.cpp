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
    static M a[2][3];
    (void)a;
}

int main()
{
    f();
    if (M::count != 6)
        return 1;
    f();
    if (M::count != 6)
        return 2;
    return 0;
}
