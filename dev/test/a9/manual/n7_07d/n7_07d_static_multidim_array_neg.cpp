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
    return 0;
}
