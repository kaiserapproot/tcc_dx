// FEAT-6A-ext4: non-member postfix operator++(T&, int) fallback.  The
// class has no member operator++, so a++ resolves to the free 2-param
// form (reference operand + int dummy).  The dummy is NAMED here because
// free-function definitions still reject unnamed params (BUG-13 fixed the
// crash only for member bodies, not the free-function declarator check);
// the implicit a++ passes 0 for it regardless of the name.
class C {
public:
    int v;
};

C operator++(C& a, int dummy) { C old; old.v = a.v; a.v = a.v + 1; return old; }

int main()
{
    C a;
    a.v = 7;
    C b = a++;                    // b.v = 7 (old), a.v = 8
    return b.v * 10 + a.v - 78;   // 70 + 8 - 78 = 0
}
