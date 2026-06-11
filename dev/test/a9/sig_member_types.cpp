// Full signature matching: same-arity overloads differing only in
// parameter type, defined out-of-class. set(int) stores a, set(double)
// stores (int)d + 100; each call must hit its own overload.
class S {
public:
    int v;
    void set(int a);
    void set(double d);
};

void S::set(int a) {
    v = a;
}

void S::set(double d) {
    v = (int)d + 100;
}

int main() {
    S s;
    int r;

    s.v = 0;
    s.set(3);
    r = s.v;            /* expect 3 */
    s.set(2.5);
    r += s.v;           /* expect 3 + 102 = 105 */
    return r - 105;
}
