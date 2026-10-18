// NEGATIVE test: must FAIL to compile.
// A double * cannot initialize a reference to int *: the pointed-to types
// differ ([dcl.init.ref]).  TCC used to make an int * temporary after only
// a warning.
typedef int *PI;

int main()
{
    const PI &r = (double *)0;  // expected error: double * for const PI &
    return r != 0;
}
