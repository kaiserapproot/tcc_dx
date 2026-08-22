// BUG-41: a member call whose ARGUMENT is itself a member call on the
// same object - `ins(begin(), ...)` - used to lose the outer call's
// `this`: cpp_member_this_pending is a single global and the nested
// call consumed it, so the outer callee read this == first argument
// (AV at run time).  SimpleList.cpp:19 `insert(begin(), n, value)`.
struct L {
    int a;
    struct It { int i; };
    It begin() { It t; t.i = 5; return t; }
    It ins(It pos, void *v)
    {
        a = 40 + pos.i;
        It r;
        r.i = 9;
        return r;
    }
    void go()
    {
        It r = ins(begin(), (void *)0);
        a = a + r.i;
    }
};
int main()
{
    L l;
    l.a = 0;
    l.go();
    return l.a == 54 ? 0 : 1;
}
