// BUG-11: a C++ keyword right after an extern "C" region was lexed in
// C mode (stale lookahead) and stayed demoted, so "class" here failed
// with "expected ';'".  Covers both the block and single-decl forms.
extern "C" { int abs(int x); }

class P {
public:
    int v;
    P(int n) { v = n; }
};

extern "C" int atoi(const char *s);

class Q {
public:
    int w;
};

int main()
{
    P p(-5);
    Q q;
    q.w = 2;
    if (abs(-3) != 3) return 1;
    if (atoi("4") != 4) return 2;
    return p.v + q.w + 3;   /* -5 + 2 + 3 = 0 */
}
