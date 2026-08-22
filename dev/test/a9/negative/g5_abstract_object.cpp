// G5 negative: an abstract class has no objects.  B inherits A's pure f()
// without overriding it, so `B b;` must be rejected too.
class A {
public:
    virtual int f() = 0;
};
class B : public A {
public:
    virtual int g() { return 1; }
};
int main()
{
    B b;
    return 0;
}
