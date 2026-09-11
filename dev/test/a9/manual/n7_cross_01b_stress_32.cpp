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

static V g00 = make_v(); static A g01;
static V g02 = make_v(); static A g03;
static V g04 = make_v(); static A g05;
static V g06 = make_v(); static A g07;
static V g08 = make_v(); static A g09;
static V g10 = make_v(); static A g11;
static V g12 = make_v(); static A g13;
static V g14 = make_v(); static A g15;
static V g16 = make_v(); static A g17;
static V g18 = make_v(); static A g19;
static V g20 = make_v(); static A g21;
static V g22 = make_v(); static A g23;
static V g24 = make_v(); static A g25;
static V g26 = make_v(); static A g27;
static V g28 = make_v(); static A g29;
static V g30 = make_v(); static A g31;

int main(void)
{
    if (seq != 32) return 1;
    if (g00.n != 1 || g01.n != 2) return 2;
    if (g30.n != 31 || g31.n != 32) return 3;
    return 0;
}
