struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

M g[4];

int main()
{
    return (M::count == 4) ? 0 : 1;
}
