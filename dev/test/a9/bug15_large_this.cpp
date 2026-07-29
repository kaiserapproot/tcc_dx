// BUG-15: a member function's writes to `this->member` were LOST when the
// object was larger than 8 bytes.  `this` was left with the struct type after
// gaddrof, so gfunc_call passed it as a by-value struct argument; for >8-byte
// objects Win64 memcpy'd the object into a temporary and passed the copy's
// address, so the method mutated the copy and the caller's object was
// unchanged.  Retyping `this` to a pointer fixed it.  Existing tests only
// used <=8-byte (1-2 member) objects, so this stayed hidden.
class Big {
public:
    int a, b, c;          // 12 bytes > 8
    void setA(int x) { a = x; }
    void setAll(int x, int y, int z) { a = x; b = y; c = z; }
    int sum() { return a + b + c; }
};

class Huge {
public:
    double p, q;          // 16 bytes
    int r;
    void set(int x) { p = x; q = x * 2; r = x * 3; }
    int total() { return (int)p + (int)q + r; }
};

int main()
{
    Big big;
    big.a = 0;
    big.setA(11);
    if (big.a != 11) return 1;          // caller sees the write
    big.setAll(1, 2, 3);
    if (big.sum() != 6) return 2;

    Huge h;
    h.set(10);                          // p=10, q=20, r=30
    if (h.total() != 60) return 3;
    return 0;
}
