// FEAT-6A-ext2 P1: T& return + chained assignment a = b = c
class Acc {
public:
    int v;
    Acc& operator=(const Acc& o);
};

Acc& Acc::operator=(const Acc& o) {
    v = o.v + 1;
    return *this;
}

int main() {
    Acc a, b, c;
    a.v = 0;
    b.v = 0;
    c.v = 10;
    a = b = c;          /* b = 11, then a = b -> 12 */
    if (b.v != 11)
        return 1;
    return a.v - 12;
}
