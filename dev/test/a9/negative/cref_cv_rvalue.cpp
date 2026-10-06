// NEGATIVE test: must FAIL to compile.
// A reference to const volatile T is not a reference to const either, so it
// cannot bind to an rvalue ([dcl.init.ref]).  The fix that made
// f(const int &) take an rvalue through a temporary accepted this too.
static int f(const volatile int &x) { return x; }

int main()
{
    return f(35);   // expected error: an rvalue for const volatile int &
}
