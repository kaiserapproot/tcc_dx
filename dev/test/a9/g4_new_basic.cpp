// G4: new Class(args) / new Class() / new Class, and delete with the
// destructor running exactly once.
int g_dtor_calls;
class P {
public:
    int v;
    P() { v = 5; }
    P(int a) { v = a; }
    P(int a, int b) { v = a * b; }
    ~P() { g_dtor_calls = g_dtor_calls + 1; }
};
int main()
{
    P* a;
    P* b;
    P* c;

    g_dtor_calls = 0;
    a = new P();
    if (a->v != 5)
        return 1;
    b = new P(7);
    if (b->v != 7)
        return 2;
    c = new P(3, 4);
    if (c->v != 12)
        return 3;
    if (a == b || b == c)
        return 4;
    delete a;
    if (g_dtor_calls != 1)
        return 5;
    delete b;
    delete c;
    if (g_dtor_calls != 3)
        return 6;
    return 0;
}
