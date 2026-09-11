struct M {
    const int x;
};

struct V {
    M m;
};

int main(void)
{
    V a, b;
    a = b;
    return 0;
}
