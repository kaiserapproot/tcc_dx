// Full signature matching, reversed declaration/definition order.
class S {
public:
    int v;
    void set(double d);
    void set(int a);
};

void S::set(double d) {
    v = (int)d + 100;
}

void S::set(int a) {
    v = a;
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
