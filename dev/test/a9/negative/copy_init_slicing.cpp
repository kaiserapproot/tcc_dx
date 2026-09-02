// FEAT-COPY-INIT fails closed on derived-to-base slicing: tpp moves struct
// arguments and return values with a memcpy rather than a copy ctor, so a
// VALUE slice would carry the derived object's vtable pointer into the base
// object.  BUG-49 (fixed separately) additionally made the converting ctor
// recurse on this operand; with that fixed the operand would be refused by
// verify_assign_cast anyway, but the dedicated diagnostic names the
// workaround (`B b(d);` or a `const B&` parameter, both of which work).
struct B {
    int b;
    B() { b = 1; }
    B(const B& o) { b = o.b + 100; }
};

struct D : public B {
    int d;
    D() { d = 2; }
};

int main()
{
    D x;
    B y = x;

    return y.b;
}