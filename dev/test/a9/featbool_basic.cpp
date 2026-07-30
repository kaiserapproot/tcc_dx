// FEAT-BOOL: the C++ `bool` type plus `true` / `false` literals.  `bool` is
// a C++ spelling of _Bool (VT_BOOL); true/false are boolean constants 1/0.
int main()
{
    bool t = true;
    bool f = false;
    bool z = (3 < 5);            // relational result assigned to bool
    bool nz = (5 < 3);
    int r = 0;
    if (!t) r += 1;             // t is true
    if (f)  r += 2;             // f is false
    if (!z) r += 4;             // 3<5 true
    if (nz) r += 8;             // 5<3 false
    // bool participates in arithmetic as 1/0
    return r + (t + z) - 2;    // 0 + (1+1) - 2 = 0
}
