// Functional cast to a CLASS type: `T(expr)` builds a ctor-initialized
// temporary - the TestCase.cpp:159 shape `return cu_String(buf);`.
// Ctor-less classes stay rejected (a9/negative/gcast_class_type.cpp).
struct Str {
    char buf[16];
    int len;
    Str(const char *s)
    {
        len = 0;
        while (s[len]) {
            buf[len] = s[len];
            len++;
        }
        buf[len] = 0;
    }
};
Str make()
{
    char b[3];

    b[0] = 'h';
    b[1] = 'i';
    b[2] = 0;
    return Str(b);              // the CPPUnit return-site shape
}
int main()
{
    Str t = Str("xyz");         // initializer position
    Str m = make();

    if (t.len != 3 || t.buf[0] != 'x' || t.buf[2] != 'z')
        return 1;
    if (m.len != 2 || m.buf[0] != 'h' || m.buf[1] != 'i')
        return 2;
    if (false) {
        Str d = Str("dead");    // dead code: type-checked, never run
        (void)d.len;
    }
    return 0;
}
