// N7-00 case B: user-declared default ctor — regression (called exactly once).
static int g_ctor_calls;

struct A {
    A(void) { ++g_ctor_calls; }
};

int main(void)
{
    A a;
    return g_ctor_calls == 1 ? 0 : 1;
}
