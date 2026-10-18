// More values that bind to a reference to const through a converted
// temporary ([dcl.init.ref]): a pointer for const bool & ([conv.bool]), the
// integer constant 0 for a reference to a member pointer ([conv.mem]), and a
// long for const int & (long and int are distinct types).  TCC rejected the
// first two, and bound const int & to the long variable itself.  A qualification
// conversion below the first level (char ** for const PCC &) still binds.
// The exit code names the first failure.

struct S { int m; int f() { return 1; } };
typedef int S::*MP;
typedef int (S::*PMF)();
typedef const char * const *PCC;

static int is_true(const bool &b) { return b; }
static int is_null_mp(const MP &p) { return p == 0; }
static int is_null_pmf(const PMF &p) { return p == 0; }
static int read_after_store(const int &r, long *pl) { *pl = 9; return r; }

int main()
{
    int x = 0;
    int *px = &x;
    int *nul = 0;
    long l = 3;
    char a[] = "abc";
    char *p = a;
    char **pp = &p;
    S s;
    MP m = &S::m;

    {
        const bool &b = px;
        const bool &bn = nul;

        if (!b) return 1;                       /* lvalue pointer -> bool */
        if (bn) return 2;                       /* a null pointer is false */
    }
    if (!is_true(&x)) return 3;                 /* rvalue argument */
    if (!is_true(px)) return 4;                 /* lvalue argument */

    {
        const MP &rm = 0;
        const PMF &rf = 0;
        const MP &rs = m;

        if (rm != 0) return 5;                  /* 0 -> null member pointer */
        if (rf != 0) return 6;
        s.m = 5;
        if (s.*rs != 5) return 7;               /* same type still binds */
    }
    if (!is_null_mp(0)) return 8;               /* ... as an argument */
    if (!is_null_pmf(0)) return 9;

    {
        const int &r = l;

        l = 9;
        if (r != 3) return 10;                  /* long -> int temporary */
        l = 3;
    }
    if (read_after_store(l, &l) != 3) return 11;    /* ... as an argument */

    {
        const PCC &r = pp;

        if (**r != 'a') return 12;              /* char ** -> const PCC */
    }
    return 0;
}
