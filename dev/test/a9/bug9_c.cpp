// BUG-9: reference variable interactions (params, returns, address-of)
class P { public: int v; };

void incp(P &p) { p.v += 1; }
void inci(int &n) { n += 10; }
int &pick(P &p) { return p.v; }

int main()
{
    P a;
    int x;
    int total;

    a.v = 1;
    P &r = a;
    incp(r);            /* ref var passed to ref param -> a.v = 2 */

    x = 5;
    int &ri = x;
    inci(ri);           /* x = 15 */
    ri = ri + 1;        /* assign through scalar ref -> x = 16 */

    pick(a) += 3;       /* ref return as lvalue -> a.v = 5 */

    int &rj = pick(r);  /* bind ref to ref-returning call */
    rj += 1;            /* a.v = 6 */

    P *pa = &r;         /* &ref == address of referent */
    pa->v += 1;         /* a.v = 7 */

    total = a.v + x;    /* 7 + 16 = 23 */
    return total - 23;
}
