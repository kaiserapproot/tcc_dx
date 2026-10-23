// NEGATIVE test: must FAIL to compile.
// The implicit copy of a class is a flat byte copy followed by a memberwise
// rebuild of the members that have copy constructors.  Array members are not
// rebuilt yet, so copying a class whose array elements have a copy
// constructor would leave two objects owning the same resources; the copy
// must be rejected rather than silently aliased.  This file covers the
// return statement; copy_array_member_init.cpp covers copy-initialization.
// (Resource has no destructor on purpose: with one, the older "return by
// value of a class requiring destruction" check would fire first.)
struct Resource {
    int *p;
    Resource() { p = 0; }
    Resource(const Resource &o) { p = o.p; }
};

struct Container {
    Resource items[2];
};

Container make(void)
{
    Container c;
    return c;       // expected error: array member with a copy constructor
}

int main(void)
{
    Container a = make();
    return a.items[0].p != 0;
}
