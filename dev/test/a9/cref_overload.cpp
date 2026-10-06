// Overload resolution with references to non-class types.  const int & is a
// pointer inside TCC, so these parameters were never scored:
// f(const int &) / f(const double &) had no viable candidate and the call
// went to whichever overload was declared last.  f and g declare the same
// pair in opposite orders so a pass cannot come from declaration order.
// The exit code names the first failure.

static int f(const int &) { return 1; }
static int f(const double &) { return 2; }
static int g(const double &) { return 2; }
static int g(const int &) { return 1; }

// A plain int lvalue binds to int & (identity) ahead of const int &; a const
// lvalue or an rvalue can only take const int &.
static int h(int &) { return 1; }
static int h(const int &) { return 2; }
static int k(const int &) { return 2; }
static int k(int &) { return 1; }

// const volatile int & takes an lvalue but no rvalue.
static int cv(const volatile int &) { return 1; }
static int cv(double) { return 2; }

static int g_value = 5;
static int &ref_to_g() { return g_value; }
static int same(const int &x) { return &x == &g_value; }
static int same(const double &) { return 0; }

struct S {
    int k;
    S() : k(0) {}
    S(const int &) : k(1) {}
    S(const double &) : k(2) {}
    int m(const int &) { return 1; }
    int m(const double &) { return 2; }
};

int main()
{
    int i = 1;
    const int c = 2;
    double d = 1.5;
    volatile int v = 0;
    S s, si(1), sd(1.5);

    if (f(1) != 1 || f(1.5) != 2) return 1;     /* rvalues */
    if (g(1) != 1 || g(1.5) != 2) return 2;     /* ... reversed order */
    if (f(i) != 1 || f(d) != 2) return 3;       /* lvalues */
    if (g(i) != 1 || g(d) != 2) return 4;
    if (h(i) != 1 || h(c) != 2 || h(3) != 2) return 5;
    if (k(i) != 1 || k(c) != 2 || k(3) != 2) return 6;
    if (cv(v) != 1 || cv(3) != 2) return 7;
    if (!same(ref_to_g())) return 8;            /* T& result binds as-is */
    if (si.k != 1 || sd.k != 2) return 9;       /* constructors */
    if (s.m(1) != 1 || s.m(1.5) != 2) return 10; /* member functions */
    return 0;
}
