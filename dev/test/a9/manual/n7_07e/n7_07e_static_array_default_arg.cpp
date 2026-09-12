struct M {
    int value;

    M(int v = 7)
    {
        value = v;
    }
};

static void touch()
{
    static M a[4];
    int i;

    for (i = 0; i < 4; i++) {
        if (a[i].value != 7)
            abort();
    }
}

int main()
{
    touch();
    touch();
    return 0;
}
