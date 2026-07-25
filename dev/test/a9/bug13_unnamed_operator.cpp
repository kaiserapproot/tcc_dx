// BUG-13: the standard postfix operator idiom operator++(int) uses an
// unnamed dummy parameter, which triggered the unnamed-member-param crash.
// Here the operators are called EXPLICITLY (a.operator++(0)); the implicit
// postfix form a++ is a separate feature (FEAT-6A-ext4).  This test locks
// in that an operator member with an unnamed param compiles and runs.
class C {
public:
    int v;
    // postfix-style signatures with an unnamed (int) dummy
    int operator++(int) { int old = v; v = v + 1; return old; }
    int operator--(int) { int old = v; v = v - 1; return old; }
};

int main()
{
    C a;
    a.v = 5;
    int x = a.operator++(0);   // x = 5, a.v = 6
    int y = a.operator--(0);   // y = 6, a.v = 5
    return x * 100 + y * 10 + a.v - 565;   // 500 + 60 + 5 - 565 = 0
}
