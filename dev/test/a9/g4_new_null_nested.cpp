// G4: `delete 0` / deleting a NULL pointer is a no-op, and a `new` whose
// constructor argument is itself a `new` must not clobber the outer
// allocation (the temp-slot spill is what makes this safe).
int g_dtor_calls;
class Inner {
public:
    int v;
    Inner(int a) { v = a; }
    ~Inner() { g_dtor_calls = g_dtor_calls + 1; }
};
class Outer {
public:
    Inner* in;
    int w;
    Outer(Inner* p, int b) { in = p; w = b; }
    ~Outer() { g_dtor_calls = g_dtor_calls + 10; }
};
int main()
{
    Outer* o;
    Inner* dead;
    char* deadArray;

    g_dtor_calls = 0;
    delete 0;
    dead = 0;
    delete dead;            /* NULL with a destructor: still a no-op */
    deadArray = 0;
    delete[] deadArray;
    if (g_dtor_calls != 0)
        return 1;

    o = new Outer(new Inner(21), 2);
    if (o->w != 2)
        return 2;
    if (o->in->v != 21)
        return 3;
    delete o->in;
    if (g_dtor_calls != 1)
        return 4;
    delete o;
    if (g_dtor_calls != 11)
        return 5;
    return 0;
}
