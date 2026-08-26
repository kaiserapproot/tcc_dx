// G3 P3: the SimpleList.cpp:114 shape - qualified return type on an
// out-of-class definition whose parameters use UNQUALIFIED class
// typedefs (resolved through cpp_qualified_class during declarator
// parsing), with run-time value checks.
class L {
public:
    typedef int value_type;
    typedef value_type* iterator;
    int a[4];
    iterator insert(iterator pos, value_type v);
};
L::iterator L::insert(iterator pos, value_type v)
{
    *pos = v;
    return pos;
}
int main()
{
    L l;
    L::iterator p;
    p = l.insert(&l.a[1], 42);
    if (l.a[1] != 42)
        return 1;
    if (p != &l.a[1])
        return 2;
    return 0;
}
