// DWARF gate (C++): a class with member functions, reference parameters and a
// local reference.  Before the fix the member functions were emitted as data
// members with DW_AT_type 0xffffffff and references with DW_AT_type 0; gdb
// dropped the whole unit, so no breakpoint in this file could be hit.
// Member pointers: a pointer to data member got DW_AT_type 0 as well (the
// same "Cannot find DIE at 0x0"), and the int parameter of a function type
// in a declarator showed up as a local variable named "<\x00>".
// dwarf_gate.bat finds the lines to break at by the marker comments.
struct acc
{
    int total;
    int count;
    acc() : total(0), count(0) {}
    void add(const int &v)
    {
        total += v;
        count += 1; // DWARF_CPP_BREAK_MEMBER
    }
};
struct pt
{
    int x;
    int y;
    int twice(int v) { return v * 2 + x; }
};
static int twice_free(int v) { return v * 2; }
static int use(const acc &a, int &out)
{
    int local = a.total;
    out = local * 2;
    return local; // DWARF_CPP_BREAK_REF
}
int main()
{
    acc a;
    int seven = 7;
    int &ref = seven;
    int thirty_five = 35;
    int doubled = 0;
    int r;
    a.add(ref);
    a.add(thirty_five);
    r = use(a, doubled);
    pt p;
    p.x = 5;
    p.y = 9;
    int pt::*pm = &pt::y;
    int (pt::*pf)(int) = &pt::twice;
    int (*fp)(int unused_param) = twice_free;
    int field = p.*pm;
    int call = (p.*pf)(3) + fp(1);
    return (r == 42 && doubled == 84 && field == 9 && call == 13) ? 0 : 1; // DWARF_CPP_BREAK_MPTR
}
