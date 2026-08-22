// G3 P5 (plan-mandated): the SimpleString.h:31 shape - a default
// argument naming a class static, called from NON-class scope, with the
// actual value checked at run time.
class S {
public:
    static const int npos;
    int last;
    void f(int n = npos);
};
const int S::npos = 41;
void S::f(int n)
{
    last = n;
}
int main()
{
    S s;
    s.f();
    if (s.last != 41)
        return 1;
    s.f(7);
    if (s.last != 7)
        return 2;
    return 0;
}
