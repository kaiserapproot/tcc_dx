// FEAT-6A-ext5: non-member (free) comparison operators fall back through
// cpp_try_free_binop, same as arithmetic free operators.
class P {
public:
    int x;
};

int operator==(const P& a, const P& b) { return a.x == b.x; }
int operator<(const P& a, const P& b)  { return a.x < b.x; }

int main()
{
    P a, b, c;
    a.x = 4; b.x = 4; c.x = 9;
    int r = 0;
    if (a == b) {} else r += 1;   // equal    -> no add
    if (a == c)   r += 2;         // not equal -> no add
    if (a < c)  {} else r += 4;   // 4 < 9    -> no add
    if (c < a)    r += 8;         // false     -> no add
    return r;                     // expect 0
}
