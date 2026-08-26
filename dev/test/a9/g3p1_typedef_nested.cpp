// G3 P1: a nested class body sees the enclosing class's typedefs
// (inner -> outer walk over cpp_enclosing_class).
class Outer {
public:
    typedef int T;
    class Inner {
    public:
        T value;
    };
};
int main()
{
    Inner i;
    i.value = 5;
    if (i.value != 5)
        return 1;
    if (sizeof(i.value) != sizeof(int))
        return 2;
    return 0;
}
