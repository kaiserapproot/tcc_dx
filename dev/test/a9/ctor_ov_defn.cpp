// ctor overload: out-of-class definitions with mem-initializer lists.
class P {
public:
    int v;
    P(int a);
    P(double d);
};

P::P(int a) : v(a) {}
P::P(double d) : v((int)d + 100) {}

int main() {
    P a(3);
    P b(2.5);
    return a.v + b.v - 105;
}
