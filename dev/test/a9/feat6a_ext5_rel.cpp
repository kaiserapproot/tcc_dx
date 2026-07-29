// FEAT-6A-ext5: member relational operators < > <= >=.
class P {
public:
    int x;
    int operator<(const P& o) const { return x < o.x; }
    int operator>(const P& o) const { return x > o.x; }
    int operator<=(const P& o) const { return x <= o.x; }
    int operator>=(const P& o) const { return x >= o.x; }
};

int main()
{
    P a, b;
    a.x = 3; b.x = 7;
    int r = 0;
    if (a < b)  {} else r += 1;
    if (b > a)  {} else r += 2;
    if (a <= a) {} else r += 4;
    if (b >= a) {} else r += 8;
    if (b < a)  r += 16;   // false -> no add
    if (a > b)  r += 32;   // false -> no add
    return r;              // expect 0
}
