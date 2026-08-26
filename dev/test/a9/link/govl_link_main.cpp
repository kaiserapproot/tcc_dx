#include "govl_link.h"

int main()
{
    Shared s;
    s.m = 0;
    if (s.f(3) != 103 || s.m != 103)
        return 1;
    if (s.f(3, 4) != 12 || s.m != 12)
        return 2;
    if (s.g() != 42)
        return 3;
    return 0;
}
