// BUG-15: the same `this`-as-by-value-struct bug affected the virtual-call
// and pointer-to-member-function paths (they set cpp_member_this the same
// way).  Verify member writes reflect for >8-byte objects through a virtual
// call and through a PMF.
class Big {
public:
    int a, b, c;                          // 12 bytes > 8
    virtual void vset(int x) { a = x; }   // virtual write
    void pset(int x) { b = x; }           // called via PMF
};

int main()
{
    Big o;
    o.a = 0; o.b = 0; o.c = 0;

    o.vset(11);                           // virtual method writes a
    if (o.a != 11) return 1;

    void (Big::*pm)(int) = &Big::pset;
    (o.*pm)(22);                          // PMF writes b
    if (o.b != 22) return 2;

    return 0;
}
