// G-OP guard: C-mode '*' and '->' are untouched by the C++ operator
// dispatch (compile gate; both hooks are cpp-gated).
struct P {
    int v;
};
int main(void)
{
    struct P a;
    struct P* p = &a;
    p->v = 5;
    return (*p).v - 5;
}
