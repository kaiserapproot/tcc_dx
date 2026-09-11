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

int main(void)
{
    X x;

    if (M::count != 4)
        return 1;
    if (x.m[0].id != 1)
        return 2;
    if (x.m[1].id != 2)
        return 3;
    if (x.m[2].id != 3)
        return 4;
    if (x.m[3].id != 4)
        return 5;
    return 0;
}
