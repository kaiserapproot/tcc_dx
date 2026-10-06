// An lvalue of another arithmetic type bound to a reference to const T goes
// through a converted temporary ([dcl.init.ref]), like an rvalue does.  TCC
// used to fail the direct lvalue binding and fall through to a plain cast:
// f(i) for f(const double &) passed the int value as the address (a hang or
// crash), and f(d) for f(const int &) did not compile.  An lvalue of the
// same type must still bind directly (same address).
// The exit code names the first failure.

static int by_int(const int &x) { return x; }
static double by_dbl(const double &x) { return x; }
static int same(const int &x, const int *p) { return &x == p; }
static int distinct(const double &a, const double &b) { return &a != &b; }

static int g_value = 1;
static int changes_g(const double &x) { g_value = 5; return x == 1.0; }

struct S {
    double sum;
    S() : sum(0) {}
    S(const double &start) : sum(start) {}
    void add(const double &v) { sum += v; }
};

int main()
{
    int i = 2;
    double d = 1.5;
    char c = 65;
    unsigned u = 7;
    bool b = true;
    float f = 0.5f;
    long long q = 9;
    volatile int v = 3;
    S s(i);

    if (by_dbl(i) != 2.0) return 1;             /* int -> double */
    if (by_int(d) != 1) return 2;               /* double -> int */
    if (by_int(c) != 65) return 3;              /* char */
    if (by_int(u) != 7) return 4;               /* unsigned */
    if (by_int(b) != 1) return 5;               /* bool */
    if (by_dbl(f) != 0.5) return 6;             /* float */
    if (by_int(q) != 9) return 7;               /* long long */
    if (by_dbl(v) != 3.0) return 8;             /* volatile int read once */
    if (!same(i, &i)) return 9;                 /* same type: no temporary */
    if (!distinct(i, i)) return 10;             /* one temporary per argument */
    if (!changes_g(g_value) || g_value != 5) return 11; /* a copy, not g */
    s.add(i);
    if (s.sum != 4.0) return 12;                /* constructor + member */
    return 0;
}
