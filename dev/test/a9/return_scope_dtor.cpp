static int dtor_log;
static int clobber_sink;
static int ref_value = 41;

int clobber_int(int n)
{
    return n + 1000;
}

double clobber_double(double n)
{
    return n + 0.25;
}

struct Guard {
    int id;

    Guard(int n) : id(n) {}

    ~Guard()
    {
        dtor_log = dtor_log * 10 + id;
        clobber_sink += clobber_int(id);
        clobber_sink += (int)clobber_double((double)id);
    }
};

struct Pair {
    int a;
    int b;
};

struct Triple {
    int a;
    int b;
    int c;
};

int return_int_from_nested_scope()
{
    Guard outer(1);
    {
        Guard inner(2);
        return outer.id * 10 + inner.id;
    }
}

double return_double()
{
    Guard guard(3);
    return 12.5;
}

int &return_reference()
{
    Guard guard(4);
    return ref_value;
}

Pair return_small_struct()
{
    Guard guard(5);
    Pair value;
    value.a = 7;
    value.b = 8;
    return value;
}

Triple return_large_struct()
{
    Guard guard(6);
    Triple value;
    value.a = 9;
    value.b = 10;
    value.c = 11;
    return value;
}

void return_void()
{
    Guard guard(7);
    return;
}

int main()
{
    int i;
    double d;
    int *p;
    Pair pair;
    Triple triple;

    dtor_log = 0;
    i = return_int_from_nested_scope();
    if (i != 12 || dtor_log != 21)
        return 1;

    dtor_log = 0;
    d = return_double();
    if (d != 12.5 || dtor_log != 3)
        return 2;

    dtor_log = 0;
    p = &return_reference();
    if (p != &ref_value || *p != 41 || dtor_log != 4)
        return 3;

    dtor_log = 0;
    pair = return_small_struct();
    if (pair.a != 7 || pair.b != 8 || dtor_log != 5)
        return 4;

    dtor_log = 0;
    triple = return_large_struct();
    if (triple.a != 9 || triple.b != 10 || triple.c != 11 || dtor_log != 6)
        return 5;

    dtor_log = 0;
    return_void();
    if (dtor_log != 7 || clobber_sink == 0)
        return 6;

    return 0;
}
