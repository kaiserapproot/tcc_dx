// FEAT-6A-ext2 P3: non-member compound assignment operator
class Acc {
public:
    int v;
};

Acc& operator+=(Acc& a, const Acc& b) {
    a.v = a.v + b.v + 1;
    return a;
}

int main() {
    Acc a, b;
    a.v = 10;
    b.v = 3;
    a += b;             /* free operator+= -> 14 */
    return a.v - 14;
}
