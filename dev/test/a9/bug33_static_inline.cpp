// BUG-33 regression: a C++ `static` member function must not receive an
// implicit `this`, whether it is defined inline in the class or outside,
// and it must keep EXTERNAL linkage (VT_STATIC on the hoisted global would
// have made it file-local).  Also covers calling one before its definition.
class U {
public:
    int inst;
    static int twice(int v) { return v * 2; }       /* inline static */
    static int thrice(int v);                        /* defined below   */
    static const int base;
    int useStatics(int v);
};
const int U::base = 10;
int U::thrice(int v) { return v * 3; }
int U::useStatics(int v)
{
    /* static members called from a non-static member body */
    return twice(v) + thrice(v) + base;
}
int main()
{
    U u;
    u.inst = 0;
    if (U::twice(4) != 8)
        return 1;
    if (U::thrice(4) != 12)
        return 2;
    if (U::base != 10)
        return 3;
    if (u.useStatics(2) != 4 + 6 + 10)
        return 4;
    return 0;
}
