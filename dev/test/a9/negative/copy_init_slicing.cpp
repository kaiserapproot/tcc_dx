// FEAT-COPY-INIT fails closed on derived-to-base slicing.  Reaching
// gen_assign_cast here would apply the G-CONV converting ctor recursively
// (the copy ctor's own `const B&` parameter is itself seen as a conversion
// target), producing 401 instead of 101 - a pre-existing defect of the
// conversion path that `take(derived)` by value shows on master too.
// master rejected this declaration form outright, so an explicit error
// keeps a wrong value from being emitted.
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