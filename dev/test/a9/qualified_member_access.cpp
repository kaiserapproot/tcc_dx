/* Qualified member access through an object: obj.B::m, p->B::m
   (C++98 5.2.5 / 10.3/12: a qualified call does not dispatch virtually).
   Expected values are g++ -std=c++98 results.  Exit code = number of the
   first failing check. */
struct B { int v; B() : v(21) {} int get() { return v; } int x; };
struct D : B { int get() { return v * 2; } int x; };

/* virtual: the qualified call runs the named class's implementation */
struct A2 { virtual int vget() { return 1; } };
struct D2 : A2 { int vget() { return 2; } int both() { return vget() * 10 + A2::vget(); } };

/* MI: the second base needs a this adjustment */
struct L3 { int l; L3() : l(1) {} int get() { return l; } };
struct R3 { int r; R3() : r(5) {} int get() { return r; } };
struct M3 : L3, R3 { int get() { return 100; } };

/* indirect base, and lookup that finds the member in a base of the
   naming class */
struct C4 : B { };
struct E4 : C4 { int get() { return 99; } };

/* overloads under the qualifier */
struct B5 { int f(int) { return 1; } int f(double) { return 2; } };
struct D5 : B5 { int f(int) { return 3; } };

/* a data member named like a class: `s.B` without `::` stays an ordinary
   member access, also right before the ':' of a conditional */
struct S6 { int B; int D; };

/* the same base twice (D7 -> L7 -> A7 and D7 -> R7 -> A7): the qualifier
   selects the path, so neither the call nor the data access is ambiguous */
struct A7 { int value; A7() : value(1) {} int get() { return value; } };
struct L7 : A7 { };
struct R7 : A7 { };
struct D7 : L7, R7 { };

int main()
{
    D d; D *pd = &d; D &rd = d;
    D2 d2; A2 *pa2 = &d2;
    M3 m3; M3 *pm3 = &m3;
    E4 e4;
    D5 d5;

    if (d.B::get() != 21) return 1;
    if (pd->B::get() != 21) return 2;
    if (rd.B::get() != 21) return 3;
    if (d.get() != 42 || d.D::get() != 42) return 4;
    if (pa2->A2::vget() != 1) return 5;       /* no virtual dispatch */
    if (pa2->vget() != 2) return 6;
    if (d2.A2::vget() != 1 || d2.both() != 21) return 7;
    d.x = 1;
    d.B::x = 2;
    pd->B::x += 1;
    if (d.x != 1 || d.B::x != 3 || ((B *)&d)->x != 3) return 8;
    if (m3.L3::get() != 1 || m3.R3::get() != 5 || m3.get() != 100) return 9;
    if (pm3->R3::get() != 5 || pm3->R3::r != 5) return 10;
    if (e4.B::get() != 21 || e4.C4::get() != 21 || e4.get() != 99) return 11;
    if (d5.B5::f(1) != 1 || d5.B5::f(1.0) != 2 || d5.f(1) != 3) return 12;
    {
        S6 s6; S6 *ps6 = &s6; int flag = 1;
        s6.B = 4;
        ps6->D = 6;
        if ((flag ? s6.B : 0) != 4 || (flag ? ps6->D : 0) != 6) return 13;
        if ((!flag ? 0 : s6.B) != 4) return 14;
    }
    {
        D7 d7; D7 *pd7 = &d7;
        ((L7 &)d7).value = 3;
        ((R7 &)d7).value = 7;
        if (d7.L7::get() != 3) return 15;
        if (d7.R7::get() != 7) return 16;
        if (pd7->R7::get() != 7 || pd7->L7::get() != 3) return 17;
        if (d7.L7::value != 3 || d7.R7::value != 7) return 18;
        d7.R7::value = 8;
        if (((R7 &)d7).value != 8 || ((L7 &)d7).value != 3) return 19;
    }
    return 0;
}
