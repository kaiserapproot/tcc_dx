// FEAT-4F: default ctor coexists with arg ctor; both forms dispatch.
class P {
public:
    int v;
    P() { v = 7; }
    P(int a) { v = a; }
};

int main() {
    P a;
    P b(3);
    return a.v + b.v - 10;
}
