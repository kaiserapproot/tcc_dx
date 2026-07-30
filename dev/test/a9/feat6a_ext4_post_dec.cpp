// FEAT-6A-ext4: member postfix operator--(int), old value by copy.
class C {
public:
    int v;
    C operator--(int) { C old; old.v = v; v = v - 1; return old; }
};

int main()
{
    C a;
    a.v = 5;
    C b = a--;                   // b.v = 5 (old), a.v = 4
    return b.v * 10 + a.v - 54;  // 50 + 4 - 54 = 0
}
