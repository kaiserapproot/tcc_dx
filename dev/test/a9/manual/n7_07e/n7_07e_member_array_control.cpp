struct M {
    static int count;

    M()
    {
        ++count;
    }
};

int M::count;

struct Holder {
    M a[4];
};

int main()
{
    Holder h;
    (void)h;
    return (M::count == 4) ? 0 : 1;
}
