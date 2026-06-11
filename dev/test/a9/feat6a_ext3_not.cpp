// FEAT-6A-ext3 P1: member operator! (incl. use in if condition)
class Box {
public:
    int n;
    int operator!();
};

int Box::operator!() {
    return n == 0;
}

int main() {
    Box e, f;
    e.n = 0;
    f.n = 9;
    if (!e && !(!f))
        return 0;
    return 1;
}
