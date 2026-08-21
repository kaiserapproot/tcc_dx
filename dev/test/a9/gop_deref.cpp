// G-OP: member "T& operator*() const" yields an lvalue - read, write,
// whole-struct assign and address-of must all act on the referenced
// object (the SimpleAutoPtr / SimpleList Iterator form).
struct P {
    int v;
};
struct It {
    P* cur;
    P& operator*() const { return *cur; }
};
int main()
{
    P a;
    P c;
    It it;
    a.v = 1;
    c.v = 9;
    it.cur = &a;
    if ((*it).v != 1)
        return 1;
    (*it).v = 5;
    if (a.v != 5)
        return 2;
    *it = c;
    if (a.v != 9)
        return 3;
    if (&(*it) != &a)
        return 4;
    return 0;
}
