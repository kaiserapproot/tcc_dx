/* A derived class member function with the same name as a base member
   function hides it (C++98 3.3.7 / 10.2), and overrides it when the base
   one is virtual even without the keyword (10.3/2).  Expected values are
   g++ -std=c++98 results.  Exit code = number of the first failing check. */
struct B1 { int v; B1() : v(21) {} int get() { return v; } };
struct D1 : B1 { int get() { return v * 2; } };

struct B2 { int v; B2() : v(21) {} int get() { return v; } };
struct D2 : B2 { virtual int get() { return v * 2; } };
struct E2 : D2 { virtual int get() { return v * 3; } };

struct B3 { int v; B3() : v(21) {} int get() { return v; } };
struct D3 : B3 { int get() { return B3::get() + 1; } };

struct A4 { int v; A4() : v(21) {} virtual int get() { return v; } };
struct D4 : A4 { int get() { return v * 2; } };
struct E4 : D4 { int get() { return v * 3; } };

struct A5 { int v; A5() : v(21) {} virtual int get() { return v; } };
struct D5 : A5 { virtual int get() { return v * 2; } };

/* out-of-class definitions */
struct B6 { int v; B6() : v(21) {} int get(); };
struct D6 : B6 { int get(); };
int B6::get() { return v; }
int D6::get() { return v * 2; }

/* the derived member calls the hidden name through this-> and unqualified */
struct B7 { int v; B7() : v(21) {} int get() { return v; } };
struct D7 : B7 { int get() { return v * 2; } int twice() { return get() + this->get(); } };

/* not an override: cv-qualification differs (only hides) */
struct A8 { virtual int get() const { return 1; } };
struct D8 : A8 { int get() { return 2; } };

/* not an override: parameter list differs (only hides) */
struct A9 { virtual int f() { return 1; } };
struct D9 : A9 { int f(int x) { return x; } };

/* data member hiding: two distinct x */
struct B10 { int x; };
struct D10 : B10 { int x; };

int main()
{
    D1 d1; B1 *b1 = &d1;
    D2 d2; B2 *b2 = &d2; E2 e2; D2 *pd2 = &e2;
    D3 d3;
    D4 d4; A4 *a4 = &d4; E4 e4; A4 *a4e = &e4; D4 *d4e = &e4;
    D5 d5; A5 *a5 = &d5;
    D6 d6; B6 *b6 = &d6;
    D7 d7;

    if (d1.get() != 42) return 1;      /* hiding, non-virtual */
    if (b1->get() != 21) return 2;     /* static type B1 */
    if (d2.get() != 42) return 3;      /* hiding, derived virtual */
    if (b2->get() != 21) return 4;     /* B2::get is not virtual */
    if (pd2->get() != 63) return 5;    /* virtual from D2 down */
    if (d3.get() != 22) return 6;      /* qualified call to the hidden one */
    if (d4.get() != 42) return 7;      /* implicit override */
    if (a4->get() != 42) return 8;
    if (a4e->get() != 63) return 9;    /* implicitly virtual two levels down */
    if (d4e->get() != 63) return 10;
    if (d5.get() != 42 || a5->get() != 42) return 11;  /* already worked */
    if (d6.get() != 42 || b6->get() != 21) return 12;
    if (d7.twice() != 84) return 13;
    {
        D8 d8; A8 *a8 = &d8;
        D9 d9; A9 *a9 = &d9;
        D10 d10; B10 *b10 = &d10;
        if (d8.get() != 2 || a8->get() != 1) return 14;
        if (d9.f(5) != 5 || a9->f() != 1) return 15;
        d10.x = 1;
        b10->x = 2;
        if (d10.x != 1 || b10->x != 2) return 16;
    }
    return 0;
}
