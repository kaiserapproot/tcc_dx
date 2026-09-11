struct M {
    static int ctor_count;

    M()
    {
        ++ctor_count;
    }
};

int M::ctor_count;

int main()
{
    M a[2] = { M(), M() };
    return (M::ctor_count == 2) ? 0 : 1;
}
