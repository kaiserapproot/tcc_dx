#include "govl_link.h"

int Shared::f(int a)
{
    m = 100 + a;
    return m;
}

int Shared::f(int a, int b)
{
    m = a * b;
    return m;
}

int Shared::g()
{
    return 42;
}
