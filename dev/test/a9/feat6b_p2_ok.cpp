// FEAT-6B-P2: const methods may read members and call const siblings;
// non-const methods still write normally.
class Box {
public:
    int v;
    int get() const { return v; }
    int twice() const { return get() * 2; }   // const -> const sibling call
    void set(int n) { v = n; }                // non-const write unchanged
};

int main()
{
    Box b;
    b.set(5);
    if (b.get() != 5) return 1;
    if (b.twice() != 10) return 2;
    b.v = 7;            // direct write on non-const object
    if (b.get() != 7) return 3;
    const Box &cb = b;  // const view: read-only access compiles
    if (cb.get() != 7) return 4;
    return 0;
}
