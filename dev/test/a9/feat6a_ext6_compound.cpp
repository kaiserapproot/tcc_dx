// FEAT-6A-ext6: compound-assignment forms %= &= |= ^= <<= >>=.  These route
// through expr_eq's struct TOK_ASSIGN branch (like ext2's += etc.), returning
// T& for chaining.
class N {
public:
    int v;
    N& operator%=(const N& o) { v %= o.v; return *this; }
    N& operator&=(const N& o) { v &= o.v; return *this; }
    N& operator|=(const N& o) { v |= o.v; return *this; }
    N& operator^=(const N& o) { v ^= o.v; return *this; }
    N& operator<<=(const N& o) { v <<= o.v; return *this; }
    N& operator>>=(const N& o) { v >>= o.v; return *this; }
};

int main()
{
    N b; b.v = 5;
    N a; a.v = 12; a %= b; if (a.v != 2)  return 1;   // 12 % 5
    N c; c.v = 12; c &= b; if (c.v != 4)  return 2;
    N d; d.v = 12; d |= b; if (d.v != 13) return 3;
    N e; e.v = 12; e ^= b; if (e.v != 9)  return 4;
    N two; two.v = 2;
    N f; f.v = 3;  f <<= two; if (f.v != 12) return 5; // 3 << 2
    N h; h.v = 40; h >>= two; if (h.v != 10) return 6; // 40 >> 2
    return 0;
}
