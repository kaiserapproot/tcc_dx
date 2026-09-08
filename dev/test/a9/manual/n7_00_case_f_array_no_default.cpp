// N7-00 case F: array of type without default ctor.
struct A {
    A(int x) { (void)x; }
};
int main(void)
{
    A a[4];
    (void)a;
    return 0;
}
