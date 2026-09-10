struct V {
    int x;
};

static V make_v(void)
{
    V v;
    v.x = 9;
    return v;
}

static const V c_identity = make_v();

int main(void)
{
    return c_identity.x == 9 ? 0 : 1;
}
