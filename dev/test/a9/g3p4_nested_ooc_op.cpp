// G3 P4: doubly qualified out-of-class definitions on a nested class -
// the SimpleList.cpp:215-236 shape (postfix operator++(int)) plus a
// plain method, with run-time value checks.
class L {
public:
    class It {
    public:
        int pos;
        It operator++(int);
        int get();
    };
};
L::It L::It::operator++(int)
{
    It old;
    old.pos = pos;
    pos = pos + 1;
    return old;
}
int L::It::get()
{
    return pos;
}
int main()
{
    L::It i;
    L::It r;
    i.pos = 5;
    r = i++;
    if (r.pos != 5)
        return 1;
    if (i.pos != 6)
        return 2;
    if (i.get() != 6)
        return 3;
    return 0;
}
