// NEGATIVE test: must FAIL to compile.
// A static reference cannot bind to a temporary here: the temporary would
// have to be static too, and TCC makes it on the stack.  TCC used to store
// 35 itself as the address, and the program hung on the first read.
int main()
{
    static const int &r = 35;   // expected error: a temporary for a static reference
    return r;
}
