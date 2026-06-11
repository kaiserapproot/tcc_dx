// FEAT-6A-ext2 P1: member operator= basic
class Acc {
public:
    int v;
    Acc& operator=(const Acc& o);
};

Acc& Acc::operator=(const Acc& o) {
    v = o.v + 100;
    return *this;
}

int main() {
    Acc a, b;
    a.v = 1;
    b.v = 2;
    a = b;                      /* member operator= -> a.v = 102 */
    return a.v - 102;
}
