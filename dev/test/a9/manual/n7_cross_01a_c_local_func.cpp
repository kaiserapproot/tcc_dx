struct V {
    int x;
};

static V make_v(void)
{
    V v;
    v.x = 7;
    return v;
}

int main(void)
{
    V v = make_v();
    return v.x == 7 ? 0 : 1;
}
