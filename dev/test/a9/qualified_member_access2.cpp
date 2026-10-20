/* Qualified member access through an object, part 2: static members,
   operator and destructor names, and a typedef as the qualifier.
   Expected values are g++ -std=c++98 results.  Exit code = number of the
   first failing check. */
static int g_side;

struct B {
    int v;
    static int count;
    B() : v(21) {}
    static int sf(int x) { return x + 1000; }
    int get() { return v; }
    int operator+(int x) { return v + x; }
    ~B() { g_side += 100; }
};
int B::count = 5;

struct D : B {
    static int sf(int x) { return x + 2000; }
    int get() { return v * 2; }
    int operator+(int x) { return v * 2 + x; }
};

/* a non-primary base, so a stray `this` argument would be visible */
struct P { int p; P() : p(9) {} };
struct M : P, B { };

typedef B BB;
typedef D DD;

static D *side_obj(D *d) { g_side += 1; return d; }

int main()
{
    D d; D *pd = &d;
    M m;

    /* static members through the qualifier */
    if (d.B::count != 5) return 1;
    d.B::count = 6;
    if (B::count != 6 || pd->B::count != 6) return 2;
    if (d.B::sf(1) != 1001 || d.sf(1) != 2001 || pd->B::sf(2) != 1002) return 3;
    if (m.B::sf(3) != 1003) return 4;
    g_side = 0;
    if (side_obj(pd)->B::sf(4) != 1004 || g_side != 1) return 5;  /* object still evaluated */

    /* operator and destructor names */
    if (d.B::operator+(1) != 22 || d.operator+(1) != 43 || d + 1 != 43) return 6;
    if (pd->B::operator+(2) != 23) return 7;
    {
        B *heap = new B;
        g_side = 0;
        heap->B::~B();               /* explicit, non-virtual */
        if (g_side != 100) return 8;
        /* not freed: delete would run the destructor a second time */
    }

    /* a typedef names the qualifying class */
    if (d.BB::get() != 21 || pd->BB::get() != 21) return 9;
    if (d.DD::get() != 42 || d.BB::count != 6) return 10;
    if (d.BB::operator+(3) != 24) return 11;
    return 0;
}
