// BUG-32a: the same default argument used by several calls.  end_macro()
// used to free the token string stored on the parameter Sym, so every call
// after the first replayed freed memory.
class C {
public:
    int v;
    static const int k;
    int f(int a = 7);
    int g(int a, int b = 3, int c = 4);
};
const int C::k = 11;
int C::f(int a) { return a; }
int C::g(int a, int b, int c) { return a * 100 + b * 10 + c; }
int main()
{
    C c;
    if (c.f() != 7)
        return 1;
    if (c.f() != 7)
        return 2;
    if (c.f() != 7)
        return 3;
    if (c.f(2) != 2)
        return 4;
    /* two defaults filled in one call, twice */
    if (c.g(1) != 134)
        return 5;
    if (c.g(1) != 134)
        return 6;
    if (c.g(1, 5) != 154)
        return 7;
    return 0;
}
