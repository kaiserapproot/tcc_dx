// An rvalue (a literal, an expression, a value of another arithmetic type)
// bound to a reference to const goes through a temporary ([dcl.init.ref]).
// TCC used to pass the value itself as the address: f(35) for
// f(const int &) dereferenced address 35 and crashed, f(35.7) did not compile,
// and a pointer rvalue for a reference to a const pointer hung.  Lvalues must still bind directly (same address), and a call that
// returns a reference must still pass that reference through.
// The exit code names the first failure.

static int by_ref(const int &x) { return x; }
static double by_dref(const double &x) { return x; }
static int by_cref(const char &c) { return c; }
static const int *addr(const int &x) { return &x; }
static int two(const int &x, const int &y) { return x * 10 + y; }
static int distinct(const int &x, const int &y) { return &x != &y; }
static int with_default(const int &x = 35) { return x; }

typedef const char *PC;
static int second(const PC &s) { return s[1]; }

static int g_value = 41;
static int &ref_to_g() { return g_value; }

struct Acc {
    int sum;
    Acc() : sum(0) {}
    Acc(const int &start) : sum(start) {}
    void add(const int &v) { sum += v; }
};

int main()
{
    int v = 30;
    const char text[] = "a#b";
    Acc a, b(7);

    if (by_ref(35) != 35) return 1;            /* literal */
    if (by_ref(v + 5) != 35) return 2;         /* expression */
    if (by_ref(35.7) != 35) return 3;          /* double converted to int */
    if (by_dref(35) != 35.0) return 4;         /* int converted to double */
    if (by_cref(65) != 65) return 5;           /* int converted to char */
    if (two(3, 5) != 35) return 6;             /* two temporaries */
    if (!distinct(1, 2)) return 7;             /* ... in separate slots */
    if (with_default() != 35 || with_default(1) != 1) return 8;
    if (addr(v) != &v) return 9;               /* lvalue: no temporary */
    if (addr(ref_to_g()) != &g_value) return 10; /* T& result passes through */
    a.add(35);
    if (a.sum != 35) return 11;                /* member function */
    if (b.sum != 7) return 12;                 /* constructor */
    if (second(text + 0) != '#') return 13;    /* pointer rvalue */
    if (second(text + 1) != 'b') return 14;
    return 0;
}