// G1: inside a member function "::name" must bind the global, never the
// same-named data member or member function (the BUG-21/BUG-22 implicit
// member lookups are bypassed for a qualified name).
int mv = 10;
int mfn()
{
    return 100;
}
class K {
public:
    int mv;
    int mfn()
    {
        return 5;
    }
    int get()
    {
        mv = 3;
        return ::mv + mv;
    }
    int call()
    {
        return ::mfn() + mfn();
    }
};
int main()
{
    K k;
    if (k.get() != 13)
        return 1;
    if (k.call() != 105)
        return 2;
    return 0;
}
