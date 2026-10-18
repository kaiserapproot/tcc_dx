// NEGATIVE test: must FAIL to compile.
// A pointer cannot initialize a reference to const int: there is no
// implicit pointer to int conversion ([dcl.init.ref]).  TCC used to accept
// this without even a warning.
int main()
{
    const int &r = (void *)0;   // expected error: void * for const int &
    return r;
}
