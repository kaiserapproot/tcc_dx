struct M {
    int x;
    M(int value = 7)
    {
        x = value;
    }
};

static int g_x;

extern "C" {
void f(void)
{
    M m(9);
    g_x = m.x;
}
}

int main()
{
    f();
    if (g_x != 9)
        return 1;
    return 0;
}
