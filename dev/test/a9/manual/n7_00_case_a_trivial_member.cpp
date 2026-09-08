// N7-00 case A: trivial member — implicit default ctor should be viable.
struct A {
    int x;
};
int main(void)
{
    A a;
    a.x = 42;
    return a.x == 42 ? 0 : 1;
}
