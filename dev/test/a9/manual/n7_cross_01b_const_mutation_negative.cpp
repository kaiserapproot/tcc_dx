struct V {
    int x;
};

static V make_v(void)
{
    V v;
    v.x = 7;
    return v;
}

static const V g = make_v();

int main(void)
{
    g.x = 9;
    return 0;
}
