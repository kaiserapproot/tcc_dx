// G-OP: member "T* operator->() const" - read, write and a const member
// function call through "->" must all reach the pointed-to object.
struct P {
    int v;
    int get() const { return v; }
};
struct It {
    P* cur;
    P* operator->() const { return cur; }
};
int main()
{
    P a;
    It it;
    a.v = 3;
    it.cur = &a;
    if (it->v != 3)
        return 1;
    it->v = 8;
    if (a.v != 8)
        return 2;
    if (it->get() != 8)
        return 3;
    return 0;
}
