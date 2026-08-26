// BUG-30 / G-OVL cross-TU case: the class is only DECLARED here, exactly
// how a real .h is used; the definitions live in govl_link_lib.cpp.  The
// call site must emit an extern reference the linker can resolve.
class Shared {
public:
    int m;
    int f(int a);
    int f(int a, int b);
    int g();
};
