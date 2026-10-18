// NEGATIVE test: must FAIL to compile.
// The same rule for an argument: f(1) for f(const PI &) is ill-formed
// ([dcl.init.ref]).  TCC used to pass an int * temporary after a warning.
typedef int *PI;

static int f(const PI &p) { return p != 0; }

int main()
{
    return f(1);    // expected error: int for const PI &
}
