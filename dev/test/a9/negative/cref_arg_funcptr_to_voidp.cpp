// NEGATIVE test: must FAIL to compile.
// The same rule for an argument: a function pointer does not convert to
// void * ([conv.ptr]).
typedef const void *PV;

static int f() { return 1; }
static int take(const PV &p) { return p != 0; }

int main()
{
    return take(&f);    // expected error: a function pointer for const PV &
}
