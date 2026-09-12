static int next_value()
{
    static int x;
    return ++x;
}

struct M {
    int value;

    M(int v = next_value()) : value(v) {}
};

static void touch()
{
    static M a[4];
    int i;

    for (i = 0; i < 4; i++) {
        if (a[i].value != i + 1)
            abort();
    }
}

int main()
{
    touch();
    touch();
    return 0;
}
