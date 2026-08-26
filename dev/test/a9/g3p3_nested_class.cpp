// G3 P3: Outer::Inner resolves as a type via the enclosing class's
// typedef list (registered when the nested class was parsed).
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
    Outer::Inner i;
    i.value = 6;
    return i.value == 6 ? 0 : 1;
}
