// BUG-30 / G-OVL negative: an argument list that matches no overload must
// still be rejected - the new declaration-based candidate set must not
// make bad calls compile.
class S {
public:
    int f(int a);
    int f(int a, int b);
};
int S::f(int a) { return 1; }
int S::f(int a, int b) { return 2; }
int main()
{
    S s;
    return s.f(1, 2, 3);
}
