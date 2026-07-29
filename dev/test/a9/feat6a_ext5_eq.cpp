// FEAT-6A-ext5: member equality operators operator== / operator!=.
// They are binary and route through the existing expr_infix ->
// cpp_try_member_binop hook, so only the operator suffixes were added.
// (Return type is int here; C++ `bool` is a separate follow-up.)
class P {
public:
    int x;
    int operator==(const P& o) const { return x == o.x; }
    int operator!=(const P& o) const { return x != o.x; }
};

int main()
{
    P a, b, c;
    a.x = 5; b.x = 5; c.x = 9;
    int r = 0;
    if (!(a == b)) r += 1;   // a==b is true  -> no add
    if (a == c)    r += 2;   // false          -> no add
    if (a != c) {} else r += 4;   // a!=c true -> no add
    if (a != b)    r += 8;   // false          -> no add
    return r;                // expect 0
}
