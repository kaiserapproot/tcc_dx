// ctor overload: inline bodies, resolved by raw argument type.
class P {
public:
    int v;
    P(int a) { v = a; }
    P(double d) { v = (int)d + 100; }
};

int main() {
    P a(3);
    P b(2.5);
    return a.v + b.v - 105;
}
