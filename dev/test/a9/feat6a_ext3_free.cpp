// FEAT-6A-ext3: free (non-member) unary operators
class Acc {
public:
    int v;
};

Acc operator-(const Acc& a) {
    Acc r;
    r.v = -a.v;
    return r;
}

int operator!(const Acc& a) {
    return a.v == 0;
}

int main() {
    Acc a, z;
    a.v = 4;
    z.v = 0;
    Acc m = -a;
    if (m.v != -4)
        return 1;
    if (!z && !(!a))
        return 0;
    return 2;
}
