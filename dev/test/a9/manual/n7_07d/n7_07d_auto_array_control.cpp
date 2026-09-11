struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

int main()
{
    M a[4];
    (void)a;
    return (M::count == 4) ? 0 : 1;
}
