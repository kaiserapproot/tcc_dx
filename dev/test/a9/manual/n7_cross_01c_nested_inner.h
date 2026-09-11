struct V { int x; };

static V make_v_nested(void)
{
    V v;
    v.x = 11;
    return v;
}

static const V g_nested = make_v_nested();
