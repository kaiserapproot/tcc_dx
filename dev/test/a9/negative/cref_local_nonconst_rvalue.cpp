// NEGATIVE test: must FAIL to compile.
// A local reference to non-const T cannot bind to an rvalue ([dcl.init.ref]).
int main()
{
    int &r = 35;    // expected error: an rvalue for int &
    return r;
}
