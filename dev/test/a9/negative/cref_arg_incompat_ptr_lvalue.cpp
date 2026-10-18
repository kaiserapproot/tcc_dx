// NEGATIVE test: must FAIL to compile.
// The same rule for an argument: a double * lvalue for const PI &.  TCC
// passed the address of the double * after only a warning.
typedef int *PI;

static int f(const PI &r) { return r != 0; }

int main()
{
    double d = 1.0;
    double *pd = &d;
    return f(pd);       // expected error: a double * lvalue for const PI &
}
