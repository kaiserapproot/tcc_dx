// ctor overload: default ctor + int ctor coexist; explicit a.P() call.
class P {
public:
    int v;
    P() { v = 7; }
    P(int a) { v = a; }
};

int main() {
    P a;
    P b(3);
    a.P();
    return a.v + b.v - 10;
}
