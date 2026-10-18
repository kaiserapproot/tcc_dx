// NEGATIVE test: must FAIL to compile.
// A reference to a pointer cannot bind an integer other than the constant 0:
// int to int * is not an implicit conversion in C++ ([dcl.init.ref]).
// TCC used to make an int * temporary after only a warning.
typedef int *PI;

int main()
{
    const PI &p = 1;    // expected error: int for const PI &
    return p != 0;
}
