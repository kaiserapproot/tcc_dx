struct V {
    int x;
};

static V make_v(void)
{
    V v;
    v.x = 7;
    return v;
}

static V g = make_v();

int main(void)
{
    return g.x == 7 ? 0 : 1;
}
