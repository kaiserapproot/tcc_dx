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
    static M m;
    (void)m;
}

int main()
{
    f();
    f();
    f();
    return (M::count == 1) ? 0 : 1;
}
