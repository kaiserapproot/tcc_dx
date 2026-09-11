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
    static M a[2] = { M(), M() };
    (void)a;
}

int main()
{
    f();
    return (M::count == 2) ? 0 : 1;
}
