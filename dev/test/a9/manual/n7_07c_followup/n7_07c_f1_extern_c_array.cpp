struct M {
    int x;
    M(int value = 7)
    {
        x = value;
    }
};

static int g_ok;

extern "C" {
void f(void)
{
    M a[4];
    int i;

    g_ok = 1;
    for (i = 0; i < 4; i++) {
        if (a[i].x != 7)
            g_ok = 0;
    }
}
}

int main()
{
    f();
    if (!g_ok)
        return 1;
    return 0;
}
