// G3 P3 regression guard: Class::member EXPRESSIONS (static data,
// static calls, statement-level assignment) keep working in a class
// that also has typedefs - the type lookup must give the tokens back
// when the qualified name is not a type.
class C {
public:
    typedef int T;
    static int sv;
    static int get() { return sv; }
};
int C::sv = 3;
int main()
{
    C::T x;
    if (C::sv != 3)
        return 1;
    C::sv = 8;
    if (C::get() != 8)
        return 2;
    x = 1;
    return x - 1;
}
