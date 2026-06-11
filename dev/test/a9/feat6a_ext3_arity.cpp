// FEAT-6A-ext3: unary and binary operator- coexist (arity resolution)
class Acc {
public:
    int v;
    Acc operator-();                /* unary  : 0 params */
    Acc operator-(const Acc& o);    /* binary : 1 param  */
};

Acc Acc::operator-() {
    Acc r;
    r.v = -v;
    return r;
}

Acc Acc::operator-(const Acc& o) {
    Acc r;
    r.v = v - o.v;
    return r;
}

int main() {
    Acc a, b;
    a.v = 10;
    b.v = 3;
    Acc u = -a;             /* unary  -> -10 */
    if (u.v != -10)
        return 1;
    Acc d = a - b;          /* binary -> 7 */
    if (d.v != 7)
        return 2;
    Acc m = -a - b;         /* unary then binary -> -13 */
    return m.v + 13;
}
