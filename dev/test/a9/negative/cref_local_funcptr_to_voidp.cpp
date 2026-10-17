// NEGATIVE test: must FAIL to compile.
// Only an object pointer converts to void *; a function pointer does not
// ([conv.ptr]), so it cannot initialize a reference to const void *.
typedef const void *PV;

static int f() { return 1; }

int main()
{
    const PV &p = &f;   // expected error: a function pointer for const PV &
    return p != 0;
}
