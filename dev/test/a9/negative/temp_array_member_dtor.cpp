// NEGATIVE test: must FAIL to compile.
// Destroying a class whose array member's elements need destruction is not
// supported yet, and a local of such a class is rejected.  A temporary of
// the class - here the discarded result of a call to a function that is only
// declared, so nothing else in this TU has met the class - must be rejected
// the same way: its implicit destructor cannot be made, and letting it
// through would just never destroy it.
struct HasD {
    int a;
    ~HasD() {}
};

struct Arr {
    HasD items[2];
};

Arr make(int v);

int use(void)
{
    make(1);        // expected error: implicit destruction of class member array is unsupported
    return 0;
}
