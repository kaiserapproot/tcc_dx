// FEAT-6A-ext4: member postfix operator++(int) returns the OLD value by
// copy, and the object is incremented.  Standard idiom uses an unnamed
// dummy (int) parameter (works thanks to BUG-13).
class C {
public:
    int v;
    C operator++(int) { C old; old.v = v; v = v + 1; return old; }
};

int main()
{
    C a;
    a.v = 5;
    C b = a++;                   // b.v = 5 (old), a.v = 6
    return b.v * 10 + a.v - 56;  // 50 + 6 - 56 = 0
}
