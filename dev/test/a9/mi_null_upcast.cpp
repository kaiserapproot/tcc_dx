// A null derived pointer must stay null when upcast to a NON-PRIMARY base.
// The MI upcast adds the base subobject offset, and adding it unconditionally
// turned `D *p = 0; B *b = p;` into b == (B*)offset - so every `if (b)` on it
// took the wrong branch.  Compiled clean, wrong at run time.
// Both the non-virtual and the virtual (secondary vtable) layouts are covered,
// together with the non-null cases that must keep working unchanged.
struct A { int a; };
struct B { int b; };
struct D : public A, public B { int d; };

struct VA { int a; virtual int fa() { return 1; } };
struct VB { int b; virtual int fb() { return 2; } };
struct VD : public VA, public VB {
    int d;
    virtual int fa() { return 10; }
    virtual int fb() { return 20; }
};

int main(void)
{
    D  dd;
    VD vd;
    D  *pd_null = 0;
    VD *pv_null = 0;
    A  *pa;
    B  *pb;
    VA *pva;
    VB *pvb;

    /* --- null must stay null --- */
    pa = pd_null;                 /* primary base: offset 0 */
    pb = pd_null;                 /* non-primary base: offset != 0 */
    if (pa != 0) return 1;
    if (pb != 0) return 1;

    pva = pv_null;
    pvb = pv_null;                /* non-primary polymorphic base */
    if (pva != 0) return 1;
    if (pvb != 0) return 1;

    /* an explicit cast must behave the same as the implicit conversion */
    if ((B *)pd_null != 0)  return 1;
    if ((VB *)pv_null != 0) return 1;

    /* --- non-null must still be adjusted to the subobject --- */
    dd.a = 1; dd.b = 2; dd.d = 3;
    pa = &dd;
    pb = &dd;
    if (pa->a != 1) return 1;
    if (pb->b != 2) return 1;
    if ((char *)pb == (char *)pa) return 1;   /* B really is at an offset */

    vd.a = 4; vd.b = 5; vd.d = 6;
    pva = &vd;
    pvb = &vd;
    if (pva->a != 4) return 1;
    if (pvb->b != 5) return 1;
    if (pva->fa() != 10) return 1;            /* dispatch through primary */
    if (pvb->fb() != 20) return 1;            /* ... and secondary vtable */
    return 0;
}
