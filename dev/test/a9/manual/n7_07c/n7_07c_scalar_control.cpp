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
    M m;
    return (M::count == 1) ? 0 : 1;
}
