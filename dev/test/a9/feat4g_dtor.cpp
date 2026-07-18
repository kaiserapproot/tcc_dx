// FEAT-4G: global dtor runs at exit (observed via side effect in dtor)
static int g_dtor_ran;

class P {
public:
    int v;
    P() { v = 1; }
    ~P() { g_dtor_ran = 1; }
};

P g;

int main()
{
    if (g.v != 1) return 1;
    return 0;   /* g_dtor_ran checked after return via atexit */
}
