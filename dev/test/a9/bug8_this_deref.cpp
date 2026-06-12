// BUG-8 regression: explicit `this` dereference in member functions.
// pointed_type(this) used to return the class tag sym's own type
// (ref=NULL) and any field walk after indir() crashed the compiler.
class Acc {
public:
    int v;
    int get();
    Acc& self();
    Acc copy();
};

int Acc::get() {
    return this->v + (*this).v;    /* -> and (*this). field walks */
}

Acc& Acc::self() {
    return *this;                  /* reference return of *this */
}

Acc Acc::copy() {
    return *this;                  /* by-value return of *this */
}

int main() {
    Acc a;
    a.v = 21;
    Acc b = a;          /* placeholder copy via memcpy path */
    b.v = 21;
    if (a.get() != 42)
        return 1;
    if (b.self().v != 21)
        return 2;
    return a.copy().v - 21;
}
