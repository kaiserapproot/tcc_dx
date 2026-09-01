// BUG-49 regression: reference binding must adjust to a secondary base and
// must not manufacture a temporary (the base copy ctor stays at zero calls).
int base_copies;

struct A {
    int av;
    A() { av = 1; }
};

struct B {
    int bv;
    B() { bv = 2; }
    B(const B& other) { bv = other.bv; base_copies++; }
};

struct D : public A, public B {
    int dv;
    D() { dv = 3; }
};

B& as_b(D& value)
{
    return value;
}

const B& as_const_b(const D& value)
{
    return value;
}

int main()
{
    D value;
    B& writable = as_b(value);
    const B& readable = as_const_b(value);

    writable.bv = 29;
    if (value.bv != 29 || readable.bv != 29)
        return 1;
    if (base_copies != 0)
        return 2;
    return 0;
}