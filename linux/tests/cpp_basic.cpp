/* C++ smoke for the Linux build: class, ctor/dtor, virtual, new/delete,
   reference, overload, operator, global ctor/dtor. */
#include <stdio.h>

static int g_trace;

class Base {
public:
    int v;
    Base(int x) : v(x) { g_trace += 1; }
    virtual ~Base() { g_trace += 100; }
    virtual int get() { return v; }
};

class Derived : public Base {
public:
    Derived(int x) : Base(x) {}
    ~Derived() { g_trace += 10; }
    /* tpp: a non-dtor override needs the explicit virtual keyword */
    virtual int get() { return v * 2; }
};

/* namespace is not supported by tpp (see tpp spec); overload only */
int add(int a, int b) { return a + b; }
double add(double a, double b) { return a + b + 0.5; }

struct Vec {
    int x, y;
    Vec(int a, int b) : x(a), y(b) {}
    Vec operator+(const Vec &o) const { return Vec(x + o.x, y + o.y); }
};

static void inc(int &r) { r++; }

class Global {
public:
    int k;
    Global() { k = 7; }
};
Global g_obj;

int main()
{
    int n = 1;
    Base *p = new Derived(21);
    if (p->get() != 42) return 1;
    delete p;
    if (g_trace != 111) return 2;
    if (add(1, 2) != 3) return 3;
    if (add(1.0, 2.0) != 3.5) return 4;
    inc(n);
    if (n != 2) return 5;
    Vec a(1, 2), b(3, 4);
    Vec c = a + b;
    if (c.x != 4 || c.y != 6) return 6;
    if (g_obj.k != 7) return 7;
    printf("cpp ok\n");
    return 0;
}
