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

/* static and non-static overloads of one name: whether `this` is passed
   depends on the overload the arguments select */
struct S7 {
    int v;
    S7() : v(7) {}
    static int f(int n) { return n + 100; }
    int f(double n) { return v + (int)n; }
};
struct T7 : S7 { };

/* a static member found through a base that occurs twice is not
   ambiguous (C++98 10.2/5); the qualified form is in
   qualified_static_ambiguous_base.cpp */
struct A8 { static int x; static int f() { return 7; } };
int A8::x = 2;
struct L8 : A8 { };
struct R8 : A8 { };
struct D8 : L8, R8 { };

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

    {
        S7 s7; S7 *ps7 = &s7; T7 t7;
        if (s7.f(4) != 104 || s7.f(3.0) != 10) return 12;
        if (ps7->f(5) != 105 || ps7->f(2.0) != 9) return 13;
        if (s7.S7::f(6) != 106 || s7.S7::f(1.0) != 8) return 14;
        if (t7.f(4) != 104 || t7.S7::f(3.0) != 10) return 15;
    }
    {
        D8 d8;
        if (d8.f() != 7) return 16;
        if (d8.x != 2) return 17;
        d8.x = 3;
        if (A8::x != 3) return 18;
    }
    return 0;
}
