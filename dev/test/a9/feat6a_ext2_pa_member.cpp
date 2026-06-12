// FEAT-6A-ext2 P2: member compound assignment operators
class Acc {
public:
    int v;
    Acc& operator+=(const Acc& o);
    Acc& operator-=(const Acc& o);
};

Acc& Acc::operator+=(const Acc& o) {
    v = v + o.v;
    return *this;
}

Acc& Acc::operator-=(const Acc& o) {
    v = v - o.v;
    return *this;
}

int main() {
    Acc a, b;
    a.v = 10;
    b.v = 3;
    a += b;             /* 13 */
    if (a.v != 13)
        return 1;
    a -= b;             /* 10 */
    if (a.v != 10)
        return 2;
    int n = 5;
    n += 2;             /* plain int compound assign must keep working */
    return n - 7;
}
