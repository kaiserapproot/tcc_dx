static int sequence;

struct V {
    int tag;
};

static V make_a(void)
{
    V v;
    v.tag = ++sequence;
    return v;
}

static V make_b(void)
{
    V v;
    v.tag = ++sequence;
    return v;
}

static V a = make_a();
static V b = make_b();

int main(void)
{
    if (a.tag != 1 || b.tag != 2)
        return 1;
    return 0;
}
