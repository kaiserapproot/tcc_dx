struct M {
    static int ctor_count;

    M()
    {
        ++ctor_count;
    }
};

int M::ctor_count;

void f()
{
    static M a[4];
    (void)a;
}

int main()
{
    f();
    return (M::ctor_count == 4) ? 0 : 1;
}
