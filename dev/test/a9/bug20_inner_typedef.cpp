// BUG-20 (cases P/Q): an inner typedef hides an outer class of the same name.
// parse_btype() used to consult struct_find() first, so the outer `struct X`
// won and `X value = 1;` was parsed as the start of a struct definition
// ("'{' expected").  These two cases are why the lookup helper has to return
// the resolved symbol instead of a mere is-a-type boolean: for `typedef int X`
// there is no struct at all to point at.
struct X { int member; };

struct Y { int only; };

// case P: the inner typedef names a non-struct type
static int inner_int(void)
{
    typedef int X;
    X value = 1;
    return value;
}

// case Q: the inner typedef names a different struct
static int inner_struct(void)
{
    typedef Y X;
    X v;
    v.only = 7;
    return v.only;
}

// the outer class must still be reachable where it is not hidden
static int outer_still_visible(void)
{
    X x;
    x.member = 5;
    return x.member;
}

int main(void)
{
    if (inner_int() != 1)
        return 1;
    if (inner_struct() != 7)
        return 1;
    if (outer_still_visible() != 5)
        return 1;
    return 0;
}
