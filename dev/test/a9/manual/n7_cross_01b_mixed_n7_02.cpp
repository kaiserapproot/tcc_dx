struct A {
    int x;
    A(void) { x = 10; }
};

struct V {
    int x;
};

static V make_v(void)
{
    V v;
    v.x = 20;
    return v;
}

struct B {
    int y;
    B(void) { y = 30; }
};

static A a;
static V b = make_v();
static B c;

int main(void)
{
    if (a.x != 10)
        return 1;
    if (b.x != 20)
        return 2;
    if (c.y != 30)
        return 3;
    return 0;
}
