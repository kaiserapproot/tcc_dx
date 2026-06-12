// FEAT-4F: implicit default ctor + automatic dtor at block end (FEAT-4E).
int g;

class P {
public:
    int v;
    P() { v = 3; g += 10; }
    ~P() { g += v; }
};

int main() {
    g = 0;
    {
        P a;
        g += a.v * 100;     /* 10 + 300 = 310 */
    }                       /* dtor: + 3 = 313 */
    return g - 313;
}
