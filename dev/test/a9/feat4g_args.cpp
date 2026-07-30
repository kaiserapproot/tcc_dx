// FEAT-4G: global ctor with arguments
class P {
public:
    int v;
    P(int n) { v = n; }
};

P g(17);

int main()
{
    return g.v - 17;
}
