// Overload resolution ranks the conversion a pointer argument needs for a
// reference-to-const pointer parameter ([over.ics.rank]): a qualification
// conversion (exact match) beats D * to B *, which beats a conversion to
// void *, which beats one to bool; a pointer that does not convert at all
// (double * for const PI &) is not viable.  TCC scored every pointer 1, so
// f(const PC &) and f(const PV &) tied for a char * and the later declaration
// won.  Each function returns 1 for the overload that must be chosen.  The
// exit code names the first failure.

typedef const char *PC;
typedef const void *PV;
typedef int *PI;

struct A { int x; };
struct B { int y; };
struct D : A, B { int z; };
typedef B *BP;

static int f(const PC &) { return 1; }
static int f(const PV &) { return 2; }
static int g(const PI &) { return 2; }
static int g(const PV &) { return 1; }
static int h(const BP &) { return 1; }
static int h(const PV &) { return 2; }
static int k(const PC &) { return 1; }
static int k(const bool &) { return 2; }
static int m(const PV &) { return 1; }
static int m(const bool &) { return 2; }

int main()
{
    char a[] = "abc";
    char *p = a;
    double x = 1.0;
    double *px = &x;
    D d;
    D *pd = &d;

    if (f(p) != 1) return 1;        /* qualification beats void * */
    if (f(a) != 1) return 2;        /* ... for an array */
    if (g(px) != 1) return 3;       /* double * does not reach const PI & */
    if (h(pd) != 1) return 4;       /* D * -> B * beats -> void * */
    if (h(&d) != 1) return 5;       /* ... for an rvalue */
    if (k(p) != 1) return 6;        /* qualification beats bool */
    if (m(p) != 1) return 7;        /* void * beats bool */
    return 0;
}
