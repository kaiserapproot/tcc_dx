struct M {
    int x;
    M(int value = 7)
    {
        x = value;
    }
};

int main()
{
    M a[2][3];
    int i;
    int expect;

    expect = 1;
    for (i = 0; i < 6; i++) {
        if (a[i / 3][i % 3].x != 7)
            return expect;
        expect++;
    }
    return 0;
}
