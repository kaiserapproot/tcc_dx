// G3 P2 (plan-mandated shadow test): a block-scope typedef in a method
// body hides the class-scope typedef of the same name.
class C {
public:
    typedef int T;
    int f();
};
int C::f()
{
    typedef char T;
    return sizeof(T) == sizeof(char) ? 0 : 1;
}
int main()
{
    C c;
    return c.f();
}
