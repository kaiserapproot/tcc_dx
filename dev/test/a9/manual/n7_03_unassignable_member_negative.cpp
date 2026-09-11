struct M {
    const int x;

    M() : x(0) {}
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
