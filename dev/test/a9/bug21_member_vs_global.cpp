// BUG-21: inside a member function, class scope outranks namespace scope.
// tcc hoists every in-class inline body to a global function, so a method of
// ANY class produced a global with that name.  Unqualified lookup checked
// sym_find() first and only fell back to the class when nothing was bound, so
// such a global shadowed this class's own data member.
// Reduced from amateras cross.h, where win_txt::clear() hid window_t::clear[4]
// and `clear[0] = 0.0f` failed with "lvalue expected".
//
// NOTE: a plain global cannot be named like any class's method yet (the
// hoisted body occupies that name and the declaration is rejected as a
// redefinition), so this test shadows via a method only - that is the case
// amateras actually hits.
struct Other {
    int v;
    void clear() { v = 0; }         // hoisted to a global named `clear`
    void n()     { v = 1; }         // ... and one named `n`
};

struct Screen {
    float clear[4];                 // array member, same name as Other::clear
    int   n;                        // scalar member, same name as Other::n

    void reset() {
        n = 3;                      // must be Screen::n
        clear[0] = 0.5f;            // BUG-21: used to resolve to Other::clear
        clear[1] = 1.5f;
        clear[2] = 2.5f;
        clear[3] = 3.5f;
    }
    float sum() { return clear[0] + clear[1] + clear[2] + clear[3]; }
};

int main(void)
{
    Other o;
    Screen s;

    o.v = 9;
    o.clear();
    if (o.v != 0)
        return 1;

    s.reset();
    if (s.n != 3)
        return 1;
    if (s.clear[0] != 0.5f || s.clear[3] != 3.5f)
        return 1;
    if (s.sum() != 8.0f)
        return 1;
    return 0;
}
