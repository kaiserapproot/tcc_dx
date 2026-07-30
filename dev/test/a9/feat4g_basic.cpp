// FEAT-4G: global default ctor runs before main
static int g_ctor_ran;

class P {
public:
    int v;
    P() { v = 42; g_ctor_ran = 1; }
};

P g;

int main()
{
    if (!g_ctor_ran) return 1;
    if (g.v != 42) return 2;
    return 0;
}
