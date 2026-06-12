// FEAT-6A-ext3 P1: member unary operator-
class Acc {
public:
    int v;
    Acc operator-();
};

Acc Acc::operator-() {
    Acc r;
    r.v = -v;
    return r;
}

int main() {
    Acc a;
    a.v = 5;
    Acc b = -a;
    if (b.v != -5)
        return 1;
    int n = 7;
    return -(-n) - 7;       /* plain int unary minus must keep working */
}
