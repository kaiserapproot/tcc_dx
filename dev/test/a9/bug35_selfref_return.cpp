// BUG-35: a member function that RETURNS ITS OWN CLASS by value, called
// BEFORE its definition (the ordinary ".h declares / .cpp defines later"
// order).  Creating the extern reference for it deep-copies the type onto
// the global stack, and that walk used to mistake the prototype sym's
// FuncAttr bits for a scope level, descend into the class's member chain,
// and recurse forever through the methods whose signatures name the class
// again - tcc died with a stack overflow and no diagnostic at all.
// This is the SimpleList::Iterator shape.
class It {
public:
    int v;
    It bump();                  /* returns its own class */
    It twice();
    int get() const;
};
/* the calls happen BEFORE the definitions: this is what creates the
   extern references and used to trigger the runaway copy */
static int use_before_definition(It& x)
{
    It a = x.bump();
    It b = a.twice();
    return b.get();
}
It It::bump()
{
    It r;
    r.v = v + 1;
    return r;
}
It It::twice()
{
    It r;
    r.v = v * 2;
    return r;
}
int It::get() const
{
    return v;
}
int main()
{
    It x;
    x.v = 5;
    if (use_before_definition(x) != 12)      /* (5+1)*2 */
        return 1;
    return 0;
}
