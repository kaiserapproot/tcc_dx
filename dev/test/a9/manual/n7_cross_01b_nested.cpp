struct M {
    int x;
};

struct V {
    M m;
};

static V make_v(void)
{
    V v;
    v.m.x = 9;
    return v;
}

static V g = make_v();

int main(void)
{
    return g.m.x == 9 ? 0 : 1;
}
