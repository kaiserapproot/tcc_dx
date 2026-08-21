// G-OP: the operator* result as a cast operand.  TestRegistry.cpp:16
// does "delete (Entry*)*p" on a void*-based list iterator; delete itself
// is G4, the cast part is verified here with run-time values.
// NOTE: "void*& operator*()" (reference-to-pointer) is a pre-existing
// declarator gap unrelated to G-OP ("int*& f()" fails too); SimpleList
// reaches that shape only through its class typedefs, which is G3.
struct P {
    int v;
};
struct W {
    void* raw;
    void* operator*() const { return raw; }
};
int main()
{
    P a;
    W w;
    P* q;
    a.v = 42;
    w.raw = &a;
    q = (P*)*w;
    if (q->v != 42)
        return 1;
    q->v = 43;
    if (a.v != 43)
        return 2;
    return 0;
}
