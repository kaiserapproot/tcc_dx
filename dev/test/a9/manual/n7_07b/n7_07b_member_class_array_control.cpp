struct M {
    static int count;
    int id;

    M()
    {
        id = ++count;
    }
};

int M::count = 0;

struct X {
    M m[4];
};

int main()
{
    X x;
    if (M::count != 4)
        return 1;
    if (x.m[0].id != 1)
        return 2;
    if (x.m[3].id != 4)
        return 3;
    return 0;
}
