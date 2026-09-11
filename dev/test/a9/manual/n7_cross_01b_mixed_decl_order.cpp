static int seq;

struct A {
    int n;
    A(void) { n = ++seq; }
};

struct V {
    int n;
};

static V make_v(void)
{
    V v;
    v.n = ++seq;
    return v;
}

static V v1 = make_v();
static A a1;
static V v2 = make_v();
static A a2;

int main(void)
{
    if (v1.n != 1) return 1;
    if (a1.n != 2) return 2;
    if (v2.n != 3) return 3;
    if (a2.n != 4) return 4;
    return 0;
}
