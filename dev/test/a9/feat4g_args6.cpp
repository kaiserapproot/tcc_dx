// FEAT-4G: 6-arg global ctor regression.
// gfunc_call stages args 5+ at [rsp+arg*8]; with the thunk's fixed
// sub rsp,0x20 they landed on the saved rbp / return address.
class P {
public:
    int v;
    P(int a, int b, int c, int d, int e, int f) { v = a + b + c + d + e + f; }
};

P g(1, 2, 3, 4, 5, 6);

int main()
{
    return g.v - 21;
}
