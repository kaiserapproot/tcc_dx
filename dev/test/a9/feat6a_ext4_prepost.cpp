// FEAT-6A-ext4: prefix operator++() (0-arg) and postfix operator++(int)
// (1 unnamed dummy) coexist; arity picks the right one.  ++a uses the
// reference-returning prefix, a++ uses the value-returning postfix.
class C {
public:
    int v;
    C& operator++() { v = v + 1; return *this; }                    // prefix
    C operator++(int) { C old; old.v = v; v = v + 1; return old; }  // postfix
};

int main()
{
    C a;
    a.v = 0;
    ++a;                          // prefix: a.v = 1
    C b = a++;                    // postfix: b.v = 1, a.v = 2
    return a.v * 10 + b.v - 21;   // 20 + 1 - 21 = 0
}
