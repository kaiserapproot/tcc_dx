struct M {
    int x;
    M(int value = 7)
    {
        x = value;
    }
};

int main()
{
    M a[4];
    int i;

    for (i = 0; i < 4; i++) {
        if (a[i].x != 7)
            return 1 + i;
    }
    return 0;
}
