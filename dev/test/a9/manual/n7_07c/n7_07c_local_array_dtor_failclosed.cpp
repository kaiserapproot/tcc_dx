struct M {
    static int live;

    M()
    {
        ++live;
    }

    ~M()
    {
        --live;
    }
};

int M::live;

int main()
{
    M a[2];
    return M::live;
}
