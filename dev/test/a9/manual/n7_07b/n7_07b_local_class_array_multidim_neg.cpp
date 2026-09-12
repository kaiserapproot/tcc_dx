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
    M a[2][3];
    return 0;
}
