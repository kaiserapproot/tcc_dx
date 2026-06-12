// FEAT-6A-ext2 P4: integrated assignment operator test
class Acc {
public:
    int v;
    Acc& operator=(const Acc& o);
    Acc& operator+=(const Acc& o);
};

Acc& Acc::operator=(const Acc& o) {
    v = o.v;
    return *this;
}

Acc& Acc::operator+=(const Acc& o) {
    v = v + o.v;
    return *this;
}

int main() {
    Acc a, b, c;
    a.v = 1;
    b.v = 2;
    c.v = 3;
    a = b;              /* member operator=  -> a.v = 2 */
    if (a.v != 2)
        return 1;
    a += c;             /* member operator+= -> a.v = 5 */
    if (a.v != 5)
        return 2;
    b = a = c;          /* chain: a = 3, b = 3 */
    if (a.v != 3 || b.v != 3)
        return 3;
    (a += b) += c;      /* T& return is assignable: a = 6 then 9 */
    return a.v - 9;
}
