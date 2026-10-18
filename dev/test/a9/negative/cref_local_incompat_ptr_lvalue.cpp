// NEGATIVE test: must FAIL to compile.
// A double * lvalue cannot initialize a reference to int *: the pointed-to
// types differ ([dcl.init.ref]).  TCC bound the reference to the double *
// variable itself.
typedef int *PI;

int main()
{
    double d = 1.0;
    double *pd = &d;
    const PI &r = pd;   // expected error: a double * lvalue for const PI &
    return r != 0;
}
