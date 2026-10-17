// A pointer lvalue binds to a reference to const pointer directly when it
// reaches the referenced type by a qualification conversion: char * for
// const PC & (GCC and MSVC bind it so in every mode, and the reference
// follows later stores to the pointer).  An array for const PC &, and D * for
// const BP & with B a base of D ([conv.ptr]), go through a temporary holding
// the converted pointer.  TCC skipped the move to the B subobject, and an
// argument D * for const BP & did not compile.  The exit code names the first
// failure.

typedef const char *PC;
typedef char *P;

struct A { int x; };
struct B { int y; };
struct D : A, B { int z; };
typedef A *AP;
typedef B *BP;

static char g_a[] = "abc";
static char g_b[] = "xyz";
static char *g_p = g_a;

static int first_after_store(const PC &r) { g_p = g_b; return *r; }
static int is_base_of(const BP &p, D *pd) { return p == (B *)pd; }
static int base_y(const BP &p) { return p->y; }

int main()
{
    char a[] = "abc";
    char b[] = "xyz";
    char *p = a;
    const PC &rc = p;
    const PC &ra = a;
    const P &rp = p;
    D d;
    D *pd = &d;
    D *pn = 0;

    if ((void *)&rc != (void *)&p) return 13;   /* binds to p itself */
    p = b;
    if (*rc != 'x') return 1;                   /* char * -> const char * */
    if (first_after_store(g_p) != 'x') return 2;    /* ... as an argument */
    a[0] = 'z';
    if (*ra != 'z') return 3;                   /* array -> const char * */
    if (*rp != 'x') return 4;                   /* same type binds to p */

    d.y = 42;
    if ((void *)(B *)&d == (void *)&d) return 5;    /* B is not at offset 0 */
    {
        const BP &rb = pd;
        const BP &rn = pn;
        const AP &rf = pd;

        pd = 0;
        if (rb != (B *)&d) return 6;            /* D * -> B *, adjusted */
        if (rb->y != 42) return 7;
        if (rn != 0) return 8;                  /* a null D * stays null */
        if (rf != (A *)&d) return 9;            /* first base, offset 0 */
        pd = &d;
    }
    if (!is_base_of(pd, &d)) return 10;         /* lvalue argument */
    if (!is_base_of(&d, &d)) return 11;         /* rvalue argument */
    if (base_y(pd) != 42) return 12;
    return 0;
}
