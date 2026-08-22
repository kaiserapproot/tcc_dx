// G4 negative: `new Class[n]` for a class with a constructor/destructor
// needs per-element construction and a stored element count; rejecting is
// better than leaving the elements silently unconstructed.
class P {
public:
    int v;
    P() { v = 1; }
};
int main()
{
    P* a;
    a = new P[4];
    return a[0].v;
}
