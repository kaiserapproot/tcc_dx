static int marker;

struct V {
    int x;
};

static V make_v(void)
{
    V v;
    marker = 123;
    v.x = 7;
    return v;
}

static V g = make_v();

int main(void)
{
    if (marker != 123)
        return 1;
    if (g.x != 7)
        return 2;
    return 0;
}
