/* Virtual slots follow the signature, not the name (C++98 10.3/2), and
   operator members follow the same hiding rule as other members.
   Expected values are g++ -std=c++98 results.  Exit code = number of the
   first failing check. */

/* 1: same name, other parameter list: a new slot, Base::get() stays */
struct B1 { virtual int get() { return 11; } };
struct D1 : B1 { virtual int get(int x) { return x + 20; } };

/* 2: overloaded base virtuals, only f(double) overridden (implicitly) */
struct B2 {
    virtual int f(int) { return 10; }
    virtual int f(double) { return 20; }
};
struct D2 : B2 { int f(double) { return 30; } };

/* 3: same, with the keyword, and overriding the first overload */
struct D3 : B2 { virtual int f(int) { return 40; } };

/* 4: cv-qualification differs: a new slot */
struct B4 { virtual int g() const { return 1; } };
struct D4 : B4 { virtual int g() { return 2; } };

/* 5: operator[] of the derived class hides the base one */
struct B5 { int operator[](int) { return 5; } };
struct D5 : B5 { int operator[](int) { return 7; } };

/* 6: binary operator hiding */
struct B6 { int v; B6() : v(1) {} int operator+(int x) { return v + x; } };
struct D6 : B6 { int operator+(int x) { return v + x + 100; } };

/* 7: covariant return type, implicit override */
struct B7 { virtual B7 *self() { return this; } virtual int id() { return 1; } };
struct D7 : B7 { D7 *self() { return this; } int id() { return 2; } };

/* 8: covariant reference return, explicit override */
struct B8 { virtual B8 &me() { return *this; } virtual int id() { return 1; } };
struct D8 : B8 { virtual D8 &me() { return *this; } int id() { return 2; } };

/* 9: overloaded virtuals: each slot gets its own overload (the vtable
   used to hold the last one for every slot) and the call picks the slot
   from the arguments, also through implicit this */
struct B9 {
    virtual int f(int) { return 10; }
    virtual int f(double) { return 20; }
    int g() { return f(1.0) + f(1) * 100; }
};
struct D9 : B9 { int f(double) { return 30; } };

/* 10: overloaded virtuals defined out of class */
struct B10 { virtual int h(int); virtual int h(double); };
int B10::h(int) { return 1; }
int B10::h(double) { return 2; }

/* 11: one overload pure */
struct B11 { virtual int k(int) = 0; virtual int k(double) { return 6; } };
struct D11 : B11 { int k(int) { return 5; } };

/* 12: secondary base: same name, other parameters is not an override */
struct L12 { virtual int a() { return 1; } };
struct R12 { virtual int get() { return 2; } };
struct M12 : L12, R12 { virtual int get(int x) { return x; } };

int main()
{
    D1 d1; B1 *b1 = &d1;
    D2 d2; B2 *b2 = &d2;
    D3 d3; B2 *b3 = &d3;
    D4 d4; B4 *b4 = &d4;
    D5 d5;
    D6 d6;
    D7 d7; B7 *b7 = &d7;
    D8 d8; B8 *b8 = &d8;

    if (b1->get() != 11) return 1;
    if (d1.get(3) != 23) return 2;
    if (b2->f(1) != 10) return 3;
    if (b2->f(1.0) != 30) return 4;
    if (b3->f(1) != 40) return 5;
    if (b3->f(1.0) != 20) return 6;
    if (b4->g() != 1 || d4.g() != 2) return 7;
    if (d5[0] != 7) return 8;
    if (d6 + 1 != 102) return 9;
    if (b7->self() != b7 || b7->self()->id() != 2) return 10;
    if (d7.self() != &d7) return 11;
    if (b8->me().id() != 2) return 12;
    {
        B9 b9; B9 *p9 = &b9; D9 d9; B9 *q9 = &d9;
        B10 b10; B10 *p10 = &b10;
        D11 d11; B11 *p11 = &d11;
        M12 m12; R12 *r12 = &m12;
        if (p9->f(1) != 10 || p9->f(1.0) != 20) return 13;
        if (b9.g() != 1020) return 14;
        if (q9->f(1) != 10 || q9->f(1.0) != 30) return 15;
        if (d9.g() != 1030) return 16;
        if (p10->h(1) != 1 || p10->h(1.0) != 2) return 17;
        if (p11->k(1) != 5 || p11->k(1.0) != 6) return 18;
        if (r12->get() != 2 || m12.get(9) != 9) return 19;
    }
    return 0;
}
