// FEAT-4F: multiple declarators `P a, b;` each get the default ctor.
class P {
public:
    int v;
    P() { v = 5; }
};

int main() {
    P a, b;
    b.v = 9;
    return a.v + b.v - 14;
}
