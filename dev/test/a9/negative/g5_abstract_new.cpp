// G5 negative: `new` of an abstract class must be rejected as well.
class A {
public:
    virtual int f() = 0;
};
int main()
{
    A* p;
    p = new A();
    return 0;
}
