// NEGATIVE test: must FAIL to compile.
// Same rule as copy_array_member_return.cpp, on a plain copy-initialization
// (no return statement involved): the flat copy would alias the array
// elements, and there is no per-element rebuild yet.
struct Resource {
    int *p;
    Resource() { p = 0; }
    Resource(const Resource &o) { p = o.p; }
};

struct Container {
    Resource items[2];
};

int main(void)
{
    Container a;
    Container b = a;    // expected error: array member with a copy constructor
    return b.items[0].p != 0;
}
