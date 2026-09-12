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
    M m;
    g_ok = (m.x == 7);
}
}

int main()
{
    f();
    if (!g_ok)
        return 1;
    return 0;
}
