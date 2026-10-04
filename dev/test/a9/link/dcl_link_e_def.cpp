// [dcl.link]: a function keeps the linkage of its first declaration.  Both
// functions below are declared extern "C" and defined outside the linkage
// specification, so they are C functions and dcl_link_e_use.c (a C TU) must
// link against them.  TCC used to give the definitions a C++ link name.
extern "C" {
int dcl_e_block(int v);
}
extern "C" int dcl_e_single(int v);

int dcl_e_block(int v) { return v + 1; }
int dcl_e_single(int v) { return v + 2; }

// An overload with a different parameter type is a different function with
// C++ linkage; it must not take over the C name.
int dcl_e_block(double v) { return (int)v + 100; }

extern "C" int dcl_e_cpp_side(void)
{
    return dcl_e_block(1.0) == 101 ? 0 : 1;
}
