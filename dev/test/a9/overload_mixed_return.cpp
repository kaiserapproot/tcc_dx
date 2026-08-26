// Overload set mixing struct and non-struct returns - SimpleList's
// `iterator insert(pos, v)` vs `void insert(pos, n, v)`.  The struct
// return of the initially bound overload used to BLOCK the deferred
// argument conversion, so the 3-arg call was cast against the 2-arg
// prototype and died with "too many arguments".  The sret slot is now
// set up after re-resolution, from the RESOLVED overload's return type.
struct L {
    int a;
    struct It { int i; };
    typedef It iterator;
    typedef unsigned int size_type;
    typedef void *value_type;
    iterator begin();
    iterator insert(iterator pos, value_type v);
    void insert(iterator pos, size_type n, value_type v);
    L(size_type n, value_type value);
};
L::L(size_type n, value_type value)
{
    a = 7;
    insert(begin(), n, value);      // must pick the 3-arg overload
}
L::iterator L::begin()
{
    It t;
    t.i = 5;
    return t;
}
L::iterator L::insert(iterator pos, value_type v)
{
    a = 1;
    return pos;
}
void L::insert(iterator pos, size_type n, value_type v)
{
    a = 20 + (int)n + pos.i;        // 20 + 3 + 5 = 28
}
int main()
{
    L l(3, (void *)0);
    if (l.a != 28)
        return 1;
    // the 2-arg (struct-returning) overload still works after the fix
    L::iterator r = l.insert(l.begin(), (void *)0);
    if (l.a != 1 || r.i != 5)
        return 2;
    return 0;
}
