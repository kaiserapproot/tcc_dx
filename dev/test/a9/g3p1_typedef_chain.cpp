// G3 P1: a class typedef may reference an earlier class typedef, and a
// typedef may alias a class type; the member using them lays out right.
class P {
public:
    int v;
};
class S {
public:
    typedef char value_type;
    typedef value_type* pointer;
    typedef P item_type;
    pointer cur;
    item_type item;
};
int main()
{
    S s;
    char c = 'x';
    s.cur = &c;
    s.item.v = 7;
    if (*s.cur != 'x')
        return 1;
    if (s.item.v != 7)
        return 2;
    if (sizeof(S) != sizeof(char*) + sizeof(P) + 4)
        return 3;
    return 0;
}
