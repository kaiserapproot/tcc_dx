// G2: "friend class Identifier;" is accepted and discarded (CPPUnit
// TestCase.h uses "friend class Logger;"); the surrounding class must
// keep working, including members declared after the friend line.
class P {
    friend class Logger;
public:
    int v;
    int get() { return v; }
};
int main()
{
    P p;
    p.v = 7;
    return p.get() - 7;
}
