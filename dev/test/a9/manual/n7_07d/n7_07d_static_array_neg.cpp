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
    return 0;
}
