static int count;

struct V {
    int id;
};

static V make_a(void)
{
    V v;
    v.id = 1;
    ++count;
    return v;
}

static V make_b(void)
{
    V v;
    v.id = 2;
    ++count;
    return v;
}

static V a = make_a();
static V b = make_b();

int main(void)
{
    if (count != 2)
        return 1;
    if (a.id != 1 || b.id != 2)
        return 2;
    return 0;
}
