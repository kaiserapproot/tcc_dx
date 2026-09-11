struct M {
    int x;
    int y;
    M(int a = 3, int b = 9)
    {
        x = a;
        y = b;
    }
};

int main()
{
    M a[4];
    int i;

    for (i = 0; i < 4; i++) {
        if (a[i].x != 3 || a[i].y != 9)
            return 1 + i;
    }
    return 0;
}
