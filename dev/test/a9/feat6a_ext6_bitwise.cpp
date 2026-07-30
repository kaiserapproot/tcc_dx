// FEAT-6A-ext6: remaining binary operators % & | ^ << >>.  Binary, so they
// route through the same expr_infix -> cpp_try_member_binop hook as ext5;
// only the operator suffixes were added.
class N {
public:
    int v;
    int operator%(const N& o) const { return v % o.v; }
    int operator&(const N& o) const { return v & o.v; }
    int operator|(const N& o) const { return v | o.v; }
    int operator^(const N& o) const { return v ^ o.v; }
    int operator<<(const N& o) const { return v << o.v; }
    int operator>>(const N& o) const { return v >> o.v; }
};

int main()
{
    N a, b;
    a.v = 12; b.v = 5;
    int r = 0;
    if ((a % b) != 2)  r += 1;    // 12 % 5  = 2
    if ((a & b) != 4)  r += 2;    // 12 & 5  = 4
    if ((a | b) != 13) r += 4;    // 12 | 5  = 13
    if ((a ^ b) != 9)  r += 8;    // 12 ^ 5  = 9
    N c, d; c.v = 3; d.v = 2;
    if ((c << d) != 12) r += 16;  // 3 << 2  = 12
    N e, f; e.v = 40; f.v = 2;
    if ((e >> f) != 10) r += 32;  // 40 >> 2 = 10
    return r;                     // expect 0
}
