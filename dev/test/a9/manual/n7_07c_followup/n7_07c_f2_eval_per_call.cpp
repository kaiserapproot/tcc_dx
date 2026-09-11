static int next_value()
{
    static int n;
    return ++n;
}

struct M {
    int x;
    M(int v = next_value()) : x(v) {}
};

int main()
{
    M a[4];

    if (a[0].x != 1)
        return 1;
    if (a[1].x != 2)
        return 2;
    if (a[2].x != 3)
        return 3;
    if (a[3].x != 4)
        return 4;
    return 0;
}
