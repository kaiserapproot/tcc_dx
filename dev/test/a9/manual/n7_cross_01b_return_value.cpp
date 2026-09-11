struct V {
    int x;
    int y;
};

static V make_v(void)
{
    V v;
    v.x = 7;
    v.y = 11;
    return v;
}

static V g = make_v();

int main(void)
{
    return (g.x == 7 && g.y == 11) ? 0 : 1;
}
