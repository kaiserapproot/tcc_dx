// FEAT-6A-ext3: prefix operator++ / operator-- with T& return
class Ctr {
public:
    int v;
    Ctr& operator++();
    Ctr& operator--();
};

Ctr& Ctr::operator++() {
    v = v + 1;
    return *this;
}

Ctr& Ctr::operator--() {
    v = v - 1;
    return *this;
}

int main() {
    Ctr c;
    c.v = 5;
    ++c;                    /* 6 */
    ++c;                    /* 7 */
    --c;                    /* 6 */
    if (c.v != 6)
        return 1;
    if ((++c).v != 7)       /* T& return usable */
        return 2;
    int n = 1;
    ++n;                    /* plain int prefix inc must keep working */
    return n - 2;
}
