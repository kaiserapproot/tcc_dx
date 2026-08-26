// G-OP: plain pointer '*' / '->' must stay on the built-in path in the
// same TU that also uses operator* / operator-> on a class type.
struct P {
    int v;
};
struct It {
    P* cur;
    P& operator*() const { return *cur; }
    P* operator->() const { return cur; }
};
int main()
{
    P a;
    P* p;
    It it;
    a.v = 4;
    p = &a;
    it.cur = &a;
    if ((*p).v != 4)
        return 1;
    if (p->v != 4)
        return 2;
    p->v = 6;
    if ((*it).v != 6)
        return 3;
    if (it->v != 6)
        return 4;
    return 0;
}
