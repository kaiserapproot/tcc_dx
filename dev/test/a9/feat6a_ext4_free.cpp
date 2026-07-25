// FEAT-6A-ext4: non-member postfix operator++(T&, int) fallback.  The
// class has no member operator++, so a++ resolves to the free 2-param
// form (reference operand + unnamed int dummy).  The unnamed dummy is the
// standard idiom and is accepted in C++ mode (BUG-13-P2).
class C {
public:
    int v;
};

C operator++(C& a, int) { C old; old.v = a.v; a.v = a.v + 1; return old; }

int main()
{
    C a;
    a.v = 7;
    C b = a++;                    // b.v = 7 (old), a.v = 8
    return b.v * 10 + a.v - 78;   // 70 + 8 - 78 = 0
}
