// FEAT-4G: multiple globals in declaration order
class P {
public:
    int v;
    P(int n) { v = n; }
};

P a(1), b(2);

int main()
{
    return a.v + b.v - 3;
}
