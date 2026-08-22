// G3 P2: a typedef inherited from a direct base resolves both in the
// derived class body (member type) and in derived method bodies.
class B {
public:
    typedef int T;
};
class D : public B {
public:
    T x;
    int m()
    {
        T y;
        y = 2;
        return y;
    }
};
int main()
{
    D d;
    d.x = 9;
    if (sizeof(d.x) != sizeof(int))
        return 1;
    if (d.x != 9)
        return 2;
    if (d.m() != 2)
        return 3;
    return 0;
}
