// NEGATIVE test: must FAIL to compile.
// A reference to non-const T cannot bind to a const lvalue: the binding would
// drop the const ([dcl.init.ref]).  TCC used to accept this and pass the
// value as the address (a crash at run time).
static int f(int &x) { return x; }

int main()
{
    const int c = 1;
    return f(c);    // expected error: a const int lvalue for int &
}
