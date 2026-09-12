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

void f()
{
    static M a[4];
    (void)a;
}

int main()
{
    f();
    return 0;
}
