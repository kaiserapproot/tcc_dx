// FEAT-BOOL + FEAT-6A-ext5: comparison operators written the idiomatic way,
// returning bool.  Confirms bool return values flow through the operator
// dispatch and into if-conditions.
class P {
public:
    int x;
    bool operator==(const P& o) const { return x == o.x; }
    bool operator!=(const P& o) const { return x != o.x; }
    bool operator<(const P& o) const { return x < o.x; }
    bool operator>=(const P& o) const { return x >= o.x; }
};

bool same(const P& a, const P& b) { return a == b; }   // bool fn over operator

int main()
{
    P a, b, c;
    a.x = 3; b.x = 7; c.x = 3;
    int r = 0;
    if (a < b)   {} else r += 1;
    if (a == c)  {} else r += 2;
    if (a != b)  {} else r += 4;
    if (b >= a)  {} else r += 8;
    if (!same(a, c)) r += 16;    // a and c equal
    if (same(a, b))  r += 32;    // a and b differ
    return r;                    // expect 0
}
