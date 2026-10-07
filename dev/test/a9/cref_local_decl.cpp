// A local reference declaration whose initializer is an rvalue, or an lvalue
// of another arithmetic type, binds through a converted temporary
// ([dcl.init.ref]), as an argument does.  TCC rejected these with "cannot
// bind reference to this initializer".  A same-type lvalue and a call that
// returns a reference must still bind directly (same address).  For a
// reference to a pointer only the integer constant 0, and a pointer that adds
// qualifiers (or to void), take a temporary; negative/cref_local_* reject the
// rest.
// The exit code names the first failure.

static int g_value = 5;
static int &ref_to_g() { return g_value; }
static int by_ref(const int &x) { return x; }
typedef int *PI;
static int is_null(const PI &p) { return p == 0; }

int main()
{
    int v = 30;
    int i = 2;
    const char text[] = "a#b";
    typedef const char *PC;
    typedef const void *PV;
    char buf[4] = "xyz";
    char *q = buf;
    int sum = 0;
    int k;

    const int &lit = 35;
    if (lit != 35) return 1;                    /* literal */
    const int &ex = v + 5;
    v = 0;
    if (ex != 35) return 2;                     /* expression, a copy */
    const double &cd = i;
    i = 7;
    if (cd != 2.0) return 3;                    /* int lvalue -> double */
    const int &ci = 35.7;
    if (ci != 35) return 4;                     /* double rvalue -> int */
    const PC &p = text + 1;
    if (*p != '#') return 5;                    /* pointer rvalue */
    const int &a = 1, &b = 2;
    if (&a == &b || a != 1 || b != 2) return 6; /* one temporary each */
    for (k = 0; k < 4; k++) {
        const int &r = k * 2;
        sum += r;
    }
    if (sum != 12) return 7;                    /* in a loop */
    const int &same = i;
    if (&same != &i) return 8;                  /* same type: no temporary */
    const int &rr = ref_to_g();
    if (&rr != &g_value) return 9;              /* T& result binds as-is */
    const int &call = by_ref(35);
    if (call != 35) return 10;                  /* call result */
    const PI &np = 0;
    if (np != 0) return 11;                     /* null pointer constant */
    const PC &qc = q + 1;
    if (*qc != 'y') return 12;                  /* char * -> const char * */
    const PV &pv = &v;
    if (pv != (const void *)&v) return 13;      /* int * -> const void * */
    if (!is_null(0)) return 14;                 /* 0 as an argument */
    return 0;
}
