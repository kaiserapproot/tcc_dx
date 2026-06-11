// FEAT-6A-ext3 P4: integrated unary operator test
class Num {
public:
    int v;
    Num operator-();                /* unary minus */
    Num operator-(const Num& o);    /* binary minus (arity coexist) */
    int operator!();
    Num& operator++();
};

Num Num::operator-() {
    Num r;
    r.v = -v;
    return r;
}

Num Num::operator-(const Num& o) {
    Num r;
    r.v = v - o.v;
    return r;
}

int Num::operator!() {
    return v == 0;
}

Num& Num::operator++() {
    v = v + 1;
    return *this;
}

int operator~(const Num& a) {
    return ~a.v;
}

int main() {
    Num a, b;
    a.v = 6;
    b.v = 2;
    Num c = -a - b;         /* -6 - 2 = -8 */
    if (c.v != -8)
        return 1;
    ++(++c);                /* -6 */
    if (c.v != -6)
        return 2;
    if (!c)                 /* member operator! -> false */
        return 3;
    if (~b != ~2)           /* free operator~ */
        return 4;
    c.v = 0;
    if (!c)
        return 0;
    return 5;
}
