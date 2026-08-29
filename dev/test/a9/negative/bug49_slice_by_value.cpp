// BUG-49 fails closed for the derived-to-base VALUE conversion.  Slicing a
// derived object into a base by value cannot be emitted correctly here: tpp
// copies struct arguments and return values with a plain memcpy (it does not
// call a copy ctor for them - measured on the same-class case), so a slice
// would also carry the derived vtable pointer into the base object.  Letting
// the converting ctor handle it instead produced 401 (BUG-49), so this form
// is rejected rather than miscompiled.
struct B {
    int b;
    B() { b = 1; }
    B(const B& o) { b = o.b + 100; }
};

struct D : public B {
    int d;
    D() { d = 2; }
};

static B slice_by_value(const D& s)
{
    return s;
}

int main()
{
    D x;

    return slice_by_value(x).b;
}