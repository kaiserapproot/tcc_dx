// DWARF gate (C++): a class with member functions, reference parameters and a
// local reference.  Before the fix the member functions were emitted as data
// members with DW_AT_type 0xffffffff and references with DW_AT_type 0; gdb
// dropped the whole unit, so no breakpoint in this file could be hit.
// dwarf_gate.bat finds the lines to break at by the two marker comments.
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
    return (r == 42 && doubled == 84) ? 0 : 1;
}
