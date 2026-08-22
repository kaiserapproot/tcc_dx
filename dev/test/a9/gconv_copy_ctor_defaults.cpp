// G-CONV: the SimpleString.h:149 shape - a copy constructor carrying
// trailing default arguments, called with ONE argument in direct
// initialization (`Str s(lhs);`), where the ctors are only DECLARED at
// the call site (definitions after; the BUG-30 machinery must see ctor
// fields through the mangled-token mapping).
struct Str {
    int len;
    int from;
    Str();
    Str(const Str& s, unsigned pos = 0, unsigned n = 999);
    Str(const char* p, unsigned n = 77);
};
static int probe(const Str& a)
{
    Str s(a);               /* copy ctor, both defaults filled */
    Str t(a, 4);            /* one default filled */
    Str u("k");             /* converting ctor, default filled */
    return s.from * 100 + t.len + u.len;
}
Str::Str() : len(0), from(0) {}
Str::Str(const Str& s, unsigned pos, unsigned n)
{
    len = (int)(pos + n);
    from = 1;
}
Str::Str(const char* p, unsigned n)
{
    len = (int)n;
    from = 2;
}
int main()
{
    Str a;
    /* s: from=1; t: len=4+999=1003; u: len=77 -> 100+1003+77 = 1180 */
    if (probe(a) != 1180)
        return 1;
    return 0;
}
