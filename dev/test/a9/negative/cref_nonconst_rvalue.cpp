// NEGATIVE test: must FAIL to compile.
// A non-const lvalue reference cannot bind to an rvalue ([dcl.init.ref]).
// TCC used to accept this and pass 35 as the address (a crash at run time).
static int f(int &x) { return x; }

int main()
{
    return f(35);   // expected error: an rvalue for int &
}