// G6 negative: a polymorphic secondary base that itself contains another
// secondary polymorphic base needs a most-derived offset-to-top.  The
// current vtable representation cannot provide it safely, so reject it.
struct A {
    virtual ~A() {}
};

struct C {
    virtual ~C() {}
};

struct B : A, C {
};

struct X {
    int x;
};

struct D : X, B {
};

int main()
{
    return sizeof(D);
}
