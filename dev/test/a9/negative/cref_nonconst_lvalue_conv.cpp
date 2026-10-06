// NEGATIVE test: must FAIL to compile.
// A non-const lvalue reference cannot bind to an lvalue of another type: it
// would need a temporary ([dcl.init.ref]).  TCC used to accept this and pass
// the int value as the address of a double (a crash at run time).
static int f(double &x) { return (int)x; }

int main()
{
    int i = 1;
    return f(i);    // expected error: an int lvalue for double &
}
