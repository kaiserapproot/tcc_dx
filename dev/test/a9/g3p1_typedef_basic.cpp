// G3 P1: class-scope typedefs are accepted, visible to later member
// declarations in the same class body, and excluded from the layout
// (the SimpleString.h:18 shape).
class S {
public:
    typedef char value_type;
    typedef unsigned int size_type;
    static const size_type npos;
    value_type buf[8];
};
int main()
{
    S s;
    if (sizeof(S) != 8)
        return 1;
    s.buf[0] = 'a';
    s.buf[7] = 'z';
    if (s.buf[0] != 'a' || s.buf[7] != 'z')
        return 2;
    return 0;
}
